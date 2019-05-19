#include<iostream>
#include<map>

using namespace std;

#define RESIZE(size) \
(size + 0x4u + 0x7u) < 16 ? \
16 : \
(size + 0x4u + 0x7u) & ~0x7u

#define CHUNK 0u
#define MEM 1u
#define PTR 2u
#define RPMC_DEBUG

class Chunk{
    public:
        unsigned int address;
        Chunk* next;
        
        virtual unsigned int type();
};

class Mem: public Chunk{
    public:
        unsigned int size;

        Mem(unsigned int _address, Chunk* _next, unsigned int _size);
        unsigned int get_addr_st();
        unsigned int get_addr_ed();
        unsigned int get_priv();
        unsigned int type();

    private:
        unsigned int init_priv();
};

class Ptr: public Chunk{
    public:
        Chunk* prev;

        Ptr(unsigned int _address, Chunk* _next, Chunk* _prev);
        unsigned int type();
};

class RPMC{
    public:
        RPMC();
        void create(unsigned int mem_address, unsigned int mem_size);
        // void link(unsigned int ptr_address, unsigned int mem_address, unsigned int mem_size);
        void link(unsigned int ptr1_address, unsigned int ptr2_address);
        void check(unsigned int ptr_address, unsigned int mem_address, unsigned int mem_size, unsigned int priv);
        void remove(unsigned int address);

    private:
        map<unsigned int, Mem*> mem_arbt;
        map<unsigned int, Ptr*> ptr_arbt;

        bool check_dup(Mem* mem);
        bool check_mem_exist(unsigned int mem_address, unsigned int mem_size);
        bool check_ptr_exist(unsigned int ptr_address);
        void link_ptr_chunk(unsigned int ptr_address, Chunk* chunk);
        void unlink(Ptr* ptr);
        Chunk* get_last_chunk(Chunk* chunk);
        Chunk* get_first_chunk(Chunk* chunk);
        void remove_mem(Mem* mem);
};

static RPMC rpmc = RPMC();
