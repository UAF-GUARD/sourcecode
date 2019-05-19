//------------------------------------------------------------------------------
// Clang rewriter sample. Demonstrates:
//
// * How to use RecursiveASTVisitor to find interesting AST nodes.
// * How to use the Rewriter API to rewrite the source code.
//
// Eli Bendersky (eliben@gmail.com)
// This code is in the public domain
//------------------------------------------------------------------------------
#include <cstdio>
#include <memory>
#include <sstream>
#include <string>

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
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
using namespace clang::driver;
using namespace clang::tooling;

// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.
class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
public:
  MyASTVisitor(Rewriter &R) : TheRewriter(R) {}

  bool VisitStmt(Stmt *s) {
    // Only care about If statements.
    if (isa<IfStmt>(s)) {
      IfStmt *IfStatement = cast<IfStmt>(s);
      Stmt *Then = IfStatement->getThen();

      TheRewriter.InsertText(Then->getBeginLoc(), "// the 'if' part\n", true,
                             true);

      Stmt *Else = IfStatement->getElse();
      if (Else)
        TheRewriter.InsertText(Else->getBeginLoc(), "// the 'else' part\n",
                               true, true);
    }
    
    s->dump();
    return true;
  }

  bool VisitFunctionDecl(FunctionDecl *f) {
    // Only function definitions (with bodies), not declarations.
    if (f->hasBody()) {
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
      ST = FuncBody->getEndLoc().getLocWithOffset(1);
      TheRewriter.InsertText(ST, SSAfter.str(), true, true);
    }

    return true;
  }

private:
  Rewriter &TheRewriter;
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R) : Visitor(R) {}

//   // Override the method that gets called for each parsed top-level
//   // declaration.
//   virtual bool HandleTopLevelDecl(DeclGroupRef DR) {
//     for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b)
//       // Traverse the declaration using our AST visitor.
//       Visitor.TraverseDecl(*b);
//     return true;
//   }
 // override this to call our ExampleVisitor on the entire source file
    virtual void HandleTranslationUnit(ASTContext &Context) {
        /* we can use ASTContext to get the TranslationUnitDecl, which is
             a single Decl that collectively represents the entire source file */
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }
private:
  MyASTVisitor Visitor;
};

// int main(int argc, char *argv[]) {
//   if (argc != 2) {
//     llvm::errs() << "Usage: rewritersample <filename>\n";
//     return 1;
//   }

//   // CompilerInstance will hold the instance of the Clang compiler for us,
//   // managing the various objects needed to run the compiler.
//   CompilerInstance TheCompInst;
//   TheCompInst.createDiagnostics();

//   LangOptions &lo = TheCompInst.getLangOpts();
//   lo.CPlusPlus = 1;

//   // Initialize target info with the default triple for our platform.
//   auto TO = std::make_shared<TargetOptions>();
//   TO->Triple = llvm::sys::getDefaultTargetTriple();
//   TargetInfo *TI =
//       TargetInfo::CreateTargetInfo(TheCompInst.getDiagnostics(), TO);
//   TheCompInst.setTarget(TI);

//   TheCompInst.createFileManager();
//   FileManager &FileMgr = TheCompInst.getFileManager();
//   TheCompInst.createSourceManager(FileMgr);
//   SourceManager &SourceMgr = TheCompInst.getSourceManager();
//   TheCompInst.createPreprocessor(TU_Module);
//   TheCompInst.createASTContext();

//   // A Rewriter helps us manage the code rewriting task.
//   Rewriter TheRewriter;
//   TheRewriter.setSourceMgr(SourceMgr, TheCompInst.getLangOpts());

//   // Set the main file handled by the source manager to the input file.
//   const FileEntry *FileIn = FileMgr.getFile(argv[1]);
//   SourceMgr.setMainFileID(
//       SourceMgr.createFileID(FileIn, SourceLocation(), SrcMgr::C_User));
//   TheCompInst.getDiagnosticClient().BeginSourceFile(
//       TheCompInst.getLangOpts(), &TheCompInst.getPreprocessor());

//   // Create an AST consumer instance which is going to get called by
//   // ParseAST.
//   MyASTConsumer TheConsumer(TheRewriter);

//   // Parse the file to AST, registering our consumer as the AST consumer.
//   ParseAST(TheCompInst.getPreprocessor(), &TheConsumer,
//            TheCompInst.getASTContext());

//   // At this point the rewriter's buffer should be full with the rewritten
//   // file contents.
//   const RewriteBuffer *RewriteBuf =
//       TheRewriter.getRewriteBufferFor(SourceMgr.getMainFileID());
//   llvm::outs() << std::string(RewriteBuf->begin(), RewriteBuf->end());

//   return 0;
// }

// int main(int argc, const char **argv) {
//     // parse the command-line args passed to your code
//     CommonOptionsParser op(argc, argv);
//     // create a new Clang Tool instance (a LibTooling environment)
//     ClangTool Tool(op.getCompilations(), op.getSourcePathList());
 
//     // run the Clang Tool, creating a new FrontendAction (explained below)
//     int result = Tool.run(newFrontendActionFactory<ExampleFrontendAction>());
 
//     errs() << "\nFound " << numFunctions << " functions.\n\n";
//     // print out the rewritten source code ("rewriter" is a global var.)
//     rewriter.getEditBuffer(rewriter.getSourceMgr().getMainFileID()).write(errs());
//     return result;
// }
class MyFrontendAction : public PluginASTAction {
public:
    MyFrontendAction() {}
    void EndSourceFileAction() override {
        TheRewriter.getEditBuffer(TheRewriter.getSourceMgr().getMainFileID())
            .write(llvm::outs());
    }

    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                    StringRef file) override {
        TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
        return llvm::make_unique<MyASTConsumer>(TheRewriter);
    }
    bool ParseArgs(const CompilerInstance &CI,
                    const std::vector<std::string> &args) override {
        return true;
    }
private:
  Rewriter TheRewriter;
};
static llvm::cl::OptionCategory MyToolCategory("my-tool options");
int main(int argc, const char **argv) {
  CommonOptionsParser op(argc, argv, MyToolCategory);
  ClangTool Tool(op.getCompilations(), op.getSourcePathList());

  return Tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}
static clang::FrontendPluginRegistry::Add<MyFrontendAction>X("rpmc", "rpmc");