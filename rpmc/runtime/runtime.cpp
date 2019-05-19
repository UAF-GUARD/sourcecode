#include"runtime.h"

unsigned int Chunk::type() { return CHUNK; }

Mem::Mem(unsigned int _address, Chunk* _next, unsigned int _size){
    this->address = _address;
    this->next = _next;
    this->size = RESIZE(_size) | init_priv();
}

unsigned int Mem::get_addr_st(){
    return this->address;
}

unsigned int Mem::get_addr_ed(){
    return this->address + (this->size & 0xfffffff8u);
}

unsigned int Mem::get_priv(){
    return this->size & 0x7u;
}

unsigned int Mem::type() { return MEM; }

unsigned int Mem::init_priv(){
    return 0x7u;
}

Ptr::Ptr(unsigned int _address, Chunk* _next, Chunk* _prev){
    this->address = _address;
    this->next = _next;
    this->prev = _prev;
}

unsigned int Ptr::type() { return PTR; }

RPMC::RPMC(){
    this->mem_arbt.clear();
    this->ptr_arbt.clear();
}

void RPMC::create(unsigned int mem_address, unsigned int mem_size){
    #ifdef RPMC_DEBUG
    cerr << "create: " << hex << mem_address << " " <<  mem_size << endl;
    #endif

    Mem* _mem = new Mem(mem_address, NULL, mem_size);
    if(this->check_dup(_mem)){
        this->mem_arbt[_mem->address] = _mem;
    }
    else{
        delete _mem;
        _mem = 0;
        throw "Memory duplication!";
    }
}

// void RPMC::link(unsigned int ptr_address, unsigned int mem_address, unsigned int mem_size){
//     #ifdef RPMC_DEBUG
//     cerr << "link: " << hex << ptr_address << " " <<  mem_address << " " <<  mem_size << endl;
//     #endif

//     if(this->check_mem_exist(mem_address, mem_size)){
//         Mem* _mem = this->mem_arbt.lower_bound(mem_address)->second;
//         this->link_ptr_chunk(ptr_address, _mem);
//     }
//     else{
//         throw "Invail memory!";
//     }
// }

void RPMC::link(unsigned int ptr1_address, unsigned int ptr2_address){
    #ifdef RPMC_DEBUG
    cerr << "link: " << hex << ptr1_address << " " <<  ptr2_address << endl;
    #endif

    if(this->check_ptr_exist(ptr2_address)){
        Ptr* _ptr = this->ptr_arbt[ptr2_address];
        this->link_ptr_chunk(ptr1_address, _ptr);
    }
    else if(this->check_mem_exist(ptr2_address, 1)){
        Mem* _mem = this->mem_arbt.lower_bound(ptr2_address)->second;
        this->link_ptr_chunk(ptr1_address, _mem);
    }
    else{
        throw "Invail pointer!";
    }
}

void RPMC::check(unsigned int ptr_address, unsigned int mem_address, unsigned int mem_size, unsigned int priv){
    #ifdef RPMC_DEBUG
    cerr << "check: " << hex << ptr_address << " " <<  mem_address << " " <<  priv << endl;
    #endif

    if(this->check_mem_exist(mem_address, mem_size)){
        Mem* _mem1 = this->mem_arbt.lower_bound(mem_address)->second;
        if(this->check_ptr_exist(ptr_address)){
            Ptr* _ptr = this->ptr_arbt[ptr_address];
            Mem* _mem2 = (Mem*)this->get_first_chunk(_ptr);
            if(_mem1 == _mem2 && (priv & (0x7u ^ _mem1->get_priv())) == 0){
                return;
            }
            else{
                throw "No privilege!";
            }
        }
        else{
            throw "Invail pointer!";
        }
    }
    else{
        throw "Invail memory!";
    }
}

void RPMC::remove(unsigned int address){
    #ifdef RPMC_DEBUG
    cerr << "remove: " << hex << address << endl;
    #endif

    if(this->mem_arbt.find(address) != this->mem_arbt.end()){
        this->remove_mem(this->mem_arbt[address]);
    }
    else if(this->check_ptr_exist(address)){
        this->remove_mem((Mem*)this->get_first_chunk(this->ptr_arbt[address]));
    }
    else{
        throw "No privilege!";
    }
}


bool RPMC::check_dup(Mem* mem){
    map<unsigned int, Mem*>::iterator _lower = this->mem_arbt.lower_bound(mem->get_addr_st());
    map<unsigned int, Mem*>::iterator _upper = this->mem_arbt.upper_bound(mem->get_addr_st());
    if(_lower != this->mem_arbt.end() && _lower->second->get_addr_ed() > mem->get_addr_st()){
        return false;
    }
    if(_upper != this->mem_arbt.end() && _upper->second->get_addr_st() < mem->get_addr_ed()){
        return false;
    }
    return true;
}

bool RPMC::check_mem_exist(unsigned int mem_address, unsigned int mem_size){
    map<unsigned int, Mem*>::iterator _lower = this->mem_arbt.lower_bound(mem_address);
    if(_lower != this->mem_arbt.end() && 
        _lower->second->get_addr_st() <= mem_address &&
        _lower->second->get_addr_ed() >= mem_address + mem_size){
        return true;
    }
    return false;
}

bool RPMC::check_ptr_exist(unsigned int ptr_address){
    if(this->ptr_arbt.find(ptr_address) != this->ptr_arbt.end()){
        return true;
    }
    return false;
}

void RPMC::link_ptr_chunk(unsigned int ptr_address, Chunk* chunk){
    if(check_ptr_exist(ptr_address)){
        this->unlink(this->ptr_arbt[ptr_address]);
    }
    else{
        this->ptr_arbt[ptr_address] = new Ptr(ptr_address, NULL, NULL);
    }
    Chunk* _last = this->get_last_chunk(chunk);
    _last->next = this->ptr_arbt[ptr_address];
    this->ptr_arbt[ptr_address]->prev = _last;
}

void RPMC::unlink(Ptr* ptr){
    Chunk* _next = ptr->next;
    Chunk* _prev = ptr->prev;
    if(_next){
        ((Ptr*)_next)->prev = _prev;
    }
    _prev->next = _next;
    ptr->next = NULL;
    ptr->prev = NULL;
}

Chunk* RPMC::get_last_chunk(Chunk* chunk){
    Chunk* _chunk = chunk;
    for(; _chunk; _chunk = _chunk->next);
    return _chunk;
}

Chunk* RPMC::get_first_chunk(Chunk* chunk){
    Chunk* _chunk = chunk;
    for(; _chunk->type() != MEM; _chunk = ((Ptr*)_chunk)->prev);
    return _chunk;
}

void RPMC::remove_mem(Mem* mem){
    Ptr* _ptr = (Ptr*)mem->next;
    delete mem;
    mem = 0;
    for(; _ptr; ){
        Ptr* _next = (Ptr*)_ptr->next;
        delete _ptr;
        _ptr = _next;
    }
}
