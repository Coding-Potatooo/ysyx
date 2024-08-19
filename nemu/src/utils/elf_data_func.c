#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <debug.h>
#include <trace.h>


func_info func_table[10000];
uint32_t func_table_cnt = 0;

/*
To implement ftrace, man elf and resort to GPT!
*/
void read_elf_header(FILE *file, Elf32_Ehdr *elf_header)
{
    fseek(file, 0, SEEK_SET);
    /*
    On success, fread() and fwrite() return the number of items read or written.  This number equals the number of bytes transferred only when size is 1.  If an error occurs, or the
    end of the file is reached, the return value is a short item count (or zero).
    */
    size_t read_count = fread(elf_header, 1, sizeof(Elf32_Ehdr), file);
    if (read_count != sizeof(Elf32_Ehdr))
    {
        Assert(0, "read elf error\n");
    }
}

/*
given elf_header, read section_headers (section_headers is an arrry, to which pointed by section_headers )
*/
void read_section_headers(FILE *file, Elf32_Ehdr *elf_header, Elf32_Shdr *section_headers)
{
    fseek(file, elf_header->e_shoff, SEEK_SET);
    size_t read_count = fread(section_headers, elf_header->e_shentsize, elf_header->e_shnum, file);
    if (read_count != elf_header->e_shnum)
    {
        Assert(0, "read elf error\n");
    }
}
/*
given elf_header, read section_names (section_names is an arrry, to which pointed by section_headers )
*/
void read_section_names(FILE *file, Elf32_Ehdr *elf_header, Elf32_Shdr *section_headers, char *section_names)
{
    Elf32_Shdr strtab_header = section_headers[elf_header->e_shstrndx];
    fseek(file, strtab_header.sh_offset, SEEK_SET);
    size_t read_count = fread(section_names, 1, strtab_header.sh_size, file);
    if (read_count != strtab_header.sh_size)
    {
        Assert(0, "read elf error\n");
    }
}

/*
given symtab_header, read symtab.
symtab_header is one of section_headers' entry.
*/
void read_symtab(FILE *file, Elf32_Shdr *symtab_header, Elf32_Sym *symtab)
{
    fseek(file, symtab_header->sh_offset, SEEK_SET);
    size_t read_count = fread(symtab, symtab_header->sh_size, 1, file);
    if (read_count != 1)
    {
        Assert(0, "read elf error\n");
    }
}

void read_strtab(FILE *file, Elf32_Shdr *strtab_header, char *strtab)
{
    fseek(file, strtab_header->sh_offset, SEEK_SET);
    size_t read_count = fread(strtab, 1, strtab_header->sh_size, file);
    if (read_count != strtab_header->sh_size)
    {
        Assert(0, "read elf error\n");
    }
}

void print_section_headers(Elf32_Ehdr *elf_header, Elf32_Shdr *section_headers, char *section_names)
{
    printf("Section Headers:\\n");
    for (int i = 0; i < elf_header->e_shnum; i++)
    {
        printf("  [%2d] %s\n", i, &section_names[section_headers[i].sh_name]);
    }
}

/*
init_elf is a function introduced to implement ftrace.
the only purpose of reading elf is just to initialize the func_table.
*/
void init_elf(const char *elf_fpath)
{
    /*
    open the elf file
    */
    Log("Read ELF file: %s", elf_fpath);
    if (elf_fpath != NULL)
    {
        FILE *file = fopen(elf_fpath, "r");
        Assert(file, "Can not open '%s'", elf_fpath);

        Elf32_Ehdr elf_header;
        read_elf_header(file, &elf_header);
        if (memcmp(elf_header.e_ident, ELFMAG, SELFMAG) != 0)
        {
            fclose(file);
            Assert(0, "Not an ELF file!\n");
        }
        Elf32_Shdr section_headers[elf_header.e_shnum];
        read_section_headers(file, &elf_header, section_headers);

        char section_names[section_headers[elf_header.e_shstrndx].sh_size];
        read_section_names(file, &elf_header, section_headers, section_names);

        Elf32_Sym *sym_table = NULL;
        char *sym_names = NULL;
        int sym_cnt = 0;

        // iterate section header table (section_headers[i]) to find symtab(sym_table) and strtab(sym_names).

        for (int i = 0; i < elf_header.e_shnum; i++)
        {
            if (section_headers[i].sh_type == SHT_SYMTAB)
            {
                sym_table = (Elf32_Sym *)malloc(section_headers[i].sh_size);
                read_symtab(file, &section_headers[i], sym_table);
                Assert(section_headers[i].sh_entsize == sizeof(Elf32_Sym), "Recheck the size of symbol table entry!!!");
                sym_cnt = section_headers[i].sh_size / section_headers[i].sh_entsize;
            }
            // if(section_headers[i].sh_type==SHT_STRTAB){  //What we want is strtab, but this may also get shstrtab!
            if (strcmp(&section_names[section_headers[i].sh_name], ".strtab") == 0)
            {
                sym_names = (char *)malloc(section_headers[i].sh_size);
                read_strtab(file, &section_headers[i], sym_names);
            }
        }

        // to initialize func_table
        for (int i = 0; i < sym_cnt; i++)
        {
            // printf("type:%d\n",sym_table[i].st_info);
            if (ELF32_ST_TYPE(sym_table[i].st_info) == STT_FUNC)
            {
                Elf32_Sym sym_i = sym_table[i];
                int name_index = 0;
                for (char *cip = sym_names+sym_i.st_name; *cip; cip++)
                {
                    func_table[func_table_cnt].name[name_index++] = *cip;
                }
                func_table[func_table_cnt].name[name_index] = 0;

                func_table[func_table_cnt].begin_addr = sym_i.st_value;
                func_table[func_table_cnt].end_addr = sym_i.st_value + sym_i.st_size;

                func_table_cnt++;
            }
        }
        free(sym_table);
        free(sym_names);

        // #define FUNCTABLE_DEBUGGING
        #ifdef FUNCTABLE_DEBUGGING
        for (int i = 0; i < func_table_index; i++)
        {
            printf("[%d],   %s, %08x,   %08x\n", i, func_table[i].name, func_table[i].begin_addr, func_table[i].end_addr);
        }
        #endif
    }
    else
    {
        Assert(0, "ELF filepath missing!");
    }
}
