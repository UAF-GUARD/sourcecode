#include <cstdio>
#include <memory>
#include <sstream>
#include <string>
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Lex/HeaderSearch.h"
#include "clang/Lex/HeaderSearchOptions.h"
#include "clang/Lex/ModuleLoader.h"
#include "clang/Lex/PreprocessorOptions.h"
#include "clang/Lex/Lexer.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;
using namespace llvm;
using namespace clang::driver;
using namespace clang::tooling;

class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor>
{
  public:
    MyASTVisitor(Rewriter &R, CompilerInstance &Instance) : TheRewriter(R), Instance(Instance) {}

    bool VisitStmt(Stmt *s)
    {
        std::string filename = Instance.getSourceManager().getFilename(s->getSourceRange().getBegin()).str();

        if (filename.empty())
            return true;

        if (filename.find("rpmc") == std::string::npos)
            return true;

        if (s->child_begin() != s->child_end())
        {
            Stmt *c1 = *(s->child_begin());
            if (clang::DeclStmt *declStmt = llvm::dyn_cast<clang::DeclStmt>(s))
            {
                VarDecl *varDecl = (VarDecl *)*declStmt->decl_begin();
                if (varDecl->getType()->isPointerType())
                {
                    while(clang::CStyleCastExpr *cStyleCastExpr = llvm::dyn_cast<clang::CStyleCastExpr>(c1))
                        c1 = *(c1->child_begin());
                    if (clang::CallExpr *callExpr = llvm::dyn_cast<clang::CallExpr>(c1))
                    {
                        if (clang::FunctionDecl *fdecl = callExpr->getDirectCallee())
                        {
                            if (fdecl->getDeclName().isIdentifier() && fdecl->getName().endswith("malloc"))
                            {
                                std::string inject = "";
                                inject += "create((unsigned int)" + this->getSourceText_V(varDecl) + ", (unsigned int)" + this->getSourceText_R(callExpr->getArg(0)->getSourceRange()) + ");\n";
                                TheRewriter.InsertText(s->getBeginLoc(), inject, true, true);
                                inject = "";
                                inject += "link((unsigned int)&" + this->getSourceText_V(varDecl) + ", (unsigned int)" + this->getSourceText_V(varDecl) + ");\n";
                                TheRewriter.InsertText(s->getBeginLoc(), inject, true, true);
                            }
                        }
                    }
                    else if (clang::ImplicitCastExpr *implicitCastExpr = llvm::dyn_cast<clang::ImplicitCastExpr>(c1))
                    {
                        Stmt *cc1 = *(c1->child_begin());
                        if (clang::Expr *expr = llvm::dyn_cast<clang::Expr>(cc1))
                        {
                            std::string inject = "";
                            inject += "link((unsigned int)&" + this->getSourceText_V(varDecl) + ", (unsigned int)&(" + this->getSourceText_R(expr->getSourceRange()) + "));\n";
                            TheRewriter.InsertText(s->getBeginLoc(), inject, true, true);
                        }
                    }
                    else if (clang::UnaryOperator *unaryOperator = llvm::dyn_cast<clang::UnaryOperator>(c1))
                    {
                        Stmt *cc1 = *(c1->child_begin());
                        if (clang::ImplicitCastExpr *implicitCastExpr = llvm::dyn_cast<clang::ImplicitCastExpr>(cc1))
                        {
                            Stmt *ccc1 = *(cc1->child_begin());
                            if (clang::DeclRefExpr *rdeclRefExpr = llvm::dyn_cast<clang::DeclRefExpr>(ccc1))
                            {
                                if (rdeclRefExpr->getType()->isPointerType())
                                {
                                    std::string inject = "";
                                    inject += "check((unsigned int)&" + this->getSourceText_R(rdeclRefExpr->getSourceRange()) + ", (unsigned int)" + this->getSourceText_R(rdeclRefExpr->getSourceRange()) + ", 1, 0x4u);\n";
                                    TheRewriter.InsertText(s->getBeginLoc(), inject, true, true);
                                    inject = "";
                                    inject += "link((unsigned int)&" + this->getSourceText_V(varDecl) + ", (unsigned int)" + this->getSourceText_V(varDecl) + ");\n";
                                    TheRewriter.InsertText(s->getBeginLoc(), inject, true, true);
                                }
                            }
                        }
                    }
                }
                else if (clang::UnaryOperator *runaryOperator = llvm::dyn_cast<clang::UnaryOperator>(c1))
                {
                    Stmt *cc1 = *(c1->child_begin());
                    if (clang::ImplicitCastExpr *implicitCastExpr = llvm::dyn_cast<clang::ImplicitCastExpr>(cc1))
                    {
                        Stmt *ccc1 = *(cc1->child_begin());
                        if (clang::DeclRefExpr *rdeclRefExpr = llvm::dyn_cast<clang::DeclRefExpr>(ccc1))
                        {
                            if (rdeclRefExpr->getType()->isPointerType())
                            {
                                std::string inject = "";
                                inject += "check((unsigned int)&" + this->getSourceText_R(rdeclRefExpr->getSourceRange()) + ", (unsigned int)" + this->getSourceText_R(rdeclRefExpr->getSourceRange()) + ", 1, 0x4u);\n";
                                TheRewriter.InsertText(s->getBeginLoc(), inject, true, true);
                            }
                        }
                    }
                }
            }
            else if (clang::BinaryOperator *binaryOperator = llvm::dyn_cast<clang::BinaryOperator>(s))
            {
                if (binaryOperator->isAssignmentOp())
                {
                    Stmt *c2 = *(++s->child_begin());
                    if (clang::DeclRefExpr *declRefExpr = llvm::dyn_cast<clang::DeclRefExpr>(c1))
                    {
                        if (declRefExpr->getType()->isPointerType())
                        {
                            while(clang::CStyleCastExpr *cStyleCastExpr = llvm::dyn_cast<clang::CStyleCastExpr>(c2))
                                c2 = *(c2->child_begin());
                            if (clang::CallExpr *callExpr = llvm::dyn_cast<clang::CallExpr>(c2))
                            {
                                if (clang::FunctionDecl *fdecl = callExpr->getDirectCallee())
                                {
                                    if (fdecl->getDeclName().isIdentifier() && fdecl->getName().endswith("malloc"))
                                    {
                                        std::string inject = "";
                                        inject += "link((unsigned int)&" + this->getSourceText_R(declRefExpr->getSourceRange()) + ", (unsigned int)" + this->getSourceText_R(declRefExpr->getSourceRange()) + ");\n";
                                        TheRewriter.InsertText(s->getBeginLoc(), inject, true, true);
                                    }
                                }
                            }
                            else if (clang::ImplicitCastExpr *implicitCastExpr = llvm::dyn_cast<clang::ImplicitCastExpr>(c2))
                            {
                                Stmt *cc2 = *(c2->child_begin());
                                if (clang::Expr *expr = llvm::dyn_cast<clang::Expr>(cc2))
                                {
                                    std::string inject = "";
                                    inject += "link((unsigned int)&" + this->getSourceText_R(declRefExpr->getSourceRange()) + ", (unsigned int)&(" + this->getSourceText_R(expr->getSourceRange()) + "));\n";
                                    TheRewriter.InsertText(s->getBeginLoc(), inject, true, true);
                                }
                            }
                            else if (clang::UnaryOperator *unaryOperator = llvm::dyn_cast<clang::UnaryOperator>(c2))
                            {
                                Stmt *cc2 = *(c2->child_begin());
                                if (clang::ImplicitCastExpr *implicitCastExpr = llvm::dyn_cast<clang::ImplicitCastExpr>(cc2))
                                {
                                    Stmt *ccc2 = *(cc2->child_begin());
                                    if (clang::DeclRefExpr *rdeclRefExpr = llvm::dyn_cast<clang::DeclRefExpr>(ccc2))
                                    {
                                        if (rdeclRefExpr->getType()->isPointerType())
                                        {
                                            std::string inject = "";
                                            inject += "check((unsigned int)&" + this->getSourceText_R(rdeclRefExpr->getSourceRange()) + ", (unsigned int)" + this->getSourceText_R(rdeclRefExpr->getSourceRange()) + ", 1, 0x4u);\n";
                                            TheRewriter.InsertText(s->getBeginLoc(), inject, true, true);
                                            inject = "";
                                            inject += "link((unsigned int)&" + this->getSourceText_R(declRefExpr->getSourceRange()) + ", (unsigned int)" + this->getSourceText_R(declRefExpr->getSourceRange()) + ");\n";
                                            TheRewriter.InsertText(s->getBeginLoc(), inject, true, true);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (clang::UnaryOperator *unaryOperator = llvm::dyn_cast<clang::UnaryOperator>(c1))
                    {
                        Stmt *cc1 = *(c1->child_begin());
                        if (clang::ImplicitCastExpr *implicitCastExpr = llvm::dyn_cast<clang::ImplicitCastExpr>(cc1))
                        {
                            Stmt *ccc1 = *(cc1->child_begin());
                            if (clang::DeclRefExpr *ldeclRefExpr = llvm::dyn_cast<clang::DeclRefExpr>(ccc1))
                            {
                                if (ldeclRefExpr->getType()->isPointerType())
                                {
                                    std::string inject = "";
                                    inject += "check((unsigned int)&" + this->getSourceText_R(ldeclRefExpr->getSourceRange()) + ", (unsigned int)" + this->getSourceText_R(ldeclRefExpr->getSourceRange()) + ", 1, 0x2u);\n";
                                    TheRewriter.InsertText(s->getBeginLoc(), inject, true, true);
                                }
                            }
                        }
                    }
                    else if (clang::UnaryOperator *runaryOperator = llvm::dyn_cast<clang::UnaryOperator>(c2))
                    {
                        Stmt *cc2 = *(c2->child_begin());
                        if (clang::ImplicitCastExpr *implicitCastExpr = llvm::dyn_cast<clang::ImplicitCastExpr>(cc2))
                        {
                            Stmt *ccc2 = *(cc2->child_begin());
                            if (clang::DeclRefExpr *rdeclRefExpr = llvm::dyn_cast<clang::DeclRefExpr>(ccc2))
                            {
                                if (rdeclRefExpr->getType()->isPointerType())
                                {
                                    std::string inject = "";
                                    inject += "check((unsigned int)&" + this->getSourceText_R(rdeclRefExpr->getSourceRange()) + ", (unsigned int)" + this->getSourceText_R(rdeclRefExpr->getSourceRange()) + ", 1, 0x4u);\n";
                                    TheRewriter.InsertText(s->getBeginLoc(), inject, true, true);
                                }
                            }
                        }
                    }
                }
            }
        }
        return true;
    }

    std::string
    getSourceText_V(VarDecl *V)
    {
        StringRef Str = V->getName();
        return Str;
    }
    std::string getSourceText_R(SourceRange R)
    {
        StringRef Str = Lexer::getSourceText(CharSourceRange::getTokenRange(R), Instance.getSourceManager(), Instance.getLangOpts());
        return Str;
    }
    bool VisitFunctionDecl(FunctionDecl *f)
    {
        // Only function definitions (with bodies), not declarations.
        if (f->hasBody())
        {
            Stmt *FuncBody = f->getBody();

            // Type name as string
            QualType QT = f->getReturnType();
            std::string TypeStr = QT.getAsString();

            // Function name
            DeclarationName DeclName = f->getNameInfo().getName();
            std::string FuncName = DeclName.getAsString();

            // Add comment before
            std::stringstream SSBefore;
            SSBefore << "// Begin function " << FuncName << " returning " << TypeStr
                     << "\n";
            SourceLocation ST = f->getSourceRange().getBegin();
            TheRewriter.InsertText(ST, SSBefore.str(), true, true);

            // And after
            std::stringstream SSAfter;
            SSAfter << "\n// End function " << FuncName;
            ST = FuncBody->getBeginLoc().getLocWithOffset(1);
            TheRewriter.InsertText(ST, SSAfter.str(), true, true);
        }

        return true;
    }

  private:
    Rewriter &TheRewriter;
    CompilerInstance &Instance;
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer
{
  public:
    MyASTConsumer(Rewriter &R, CompilerInstance &CI) : Visitor(R, CI) {}

    // override this to call our ExampleVisitor on the entire source file
    virtual void HandleTranslationUnit(ASTContext &Context)
    {
        /* we can use ASTContext to get the TranslationUnitDecl, which is
             a single Decl that collectively represents the entire source file */
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }

  private:
    MyASTVisitor Visitor;
};

class MyFrontendAction : public PluginASTAction
{
  public:
    MyFrontendAction() {}
    void EndSourceFileAction() override
    {
        TheRewriter.getEditBuffer(TheRewriter.getSourceMgr().getMainFileID())
            .write(llvm::outs());
    }

    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                   StringRef file) override
    {
        TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
        return llvm::make_unique<MyASTConsumer>(TheRewriter, CI);
    }
    bool ParseArgs(const CompilerInstance &CI,
                   const std::vector<std::string> &args) override
    {
        return true;
    }

  private:
    Rewriter TheRewriter;
};
static llvm::cl::OptionCategory MyToolCategory("my-tool options");
int main(int argc, const char **argv)
{
    CommonOptionsParser op(argc, argv, MyToolCategory);
    ClangTool Tool(op.getCompilations(), op.getSourcePathList());

    return Tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}
static clang::FrontendPluginRegistry::Add<MyFrontendAction> X("rpmc", "rpmc");
