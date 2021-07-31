﻿

#include "uc_util.h"
#include "mcore/mcore.h"
#include <assert.h>

#define align(x,n)              ((x + n - 1) & ~(n - 1))

static void             ur_elf_addr_fix(uc_runtime_t *t);
static uc_pos_t         ur__alloc(uc_runtime_t *r, struct uc_area *area, int size, int align_size);

#define ALIGN_FILL(shift)               shift,  1 << (shift), (~((1 << SHIFT) -1))

struct uc_area  default_areas[] = {
    { UC_AREA_TEXT, 0,  0x10000,    0, 1   * MB, UC_PROT_READ | UC_PROT_EXEC,  12, 1 << 12, 0 },
    { UC_AREA_DATA, 0,  0x200000,   0, 128 * KB, UC_PROT_READ | UC_PROT_WRITE, 12, 1 << 12, 0 },
    { UC_AREA_BSS,  0,  0x300000,   0, 4   * KB, UC_PROT_READ | UC_PROT_WRITE, 12, 1 << 12, 0 },
    { UC_AREA_HEAP, 0,  0x400000,   0, 1   * MB, UC_PROT_READ | UC_PROT_WRITE, 12, 1 << 12, 0 },
    { UC_AREA_STACK,0,  0x500000,   0, 128 * KB, UC_PROT_READ | UC_PROT_WRITE, 12, 1 << 12, 0 },
};

uc_runtime_t*   uc_runtime_new(uc_engine *uc, const char *soname, int stack_size, int heap_size)
{
    uc_runtime_t *ur;
    uc_err err;
    struct uc_area *area;
    int i;

    ur = calloc(1, sizeof (ur[0]) + strlen(soname));
    if (!ur)
        return NULL;

    ur->fdata = file_load(soname, &ur->flen);
    if (!ur->fdata) {
        print_err ("err: %s failed when file_load. %s:%d\r\n",  __func__, __FILE__, __LINE__);
        return NULL;
    }

    ur->uc = uc;
    strcpy(ur->soname, soname);

    for (i = 0; i < count_of_array(default_areas); i++) {
        area = &ur->areas[default_areas[i].type];
        *area = default_areas[i];

        if ((err = uc_mem_map(uc, area->begin, area->size, area->perms))) {
            print_err ("err: %s  failed with (%d:%s)uc_mem_map. %s:%d\r\n",  __func__, err, uc_strerror(err), __FILE__, __LINE__);
            return NULL;
        }

        area->end = area->begin + area->size - 1;
    }

    ur->elf_load_addr = ur__alloc(ur, ur_area_text(ur), ur->flen, ur_area_text(ur)->align_size);
    uc_mem_write(uc, ur->elf_load_addr, ur->fdata, ur->flen);

    ur_elf_addr_fix(ur);

    return ur;
}

static uc_pos_t            ur__alloc(uc_runtime_t *r, struct uc_area *area, int size, int align_size)
{
    int ind = area->index;

    if ((ind + size) > area->size)
        return 0;

    if (!align_size)
        align_size = 1;

    area->index += align(size, align_size);

    return area->begin + ind;
}

uc_pos_t            ur_malloc(uc_runtime_t *r, int size)
{
    return ur__alloc(r, ur_area_heap(r), size, 8);
}

uc_pos_t           ur_calloc(uc_runtime_t *uc, int elmnum, int elmsiz)
{
    return 0;
}

uc_pos_t            ur_text_start(uc_runtime_t *r)
{
    return ur_area_text(r)->begin;
}

uc_pos_t        ur_text_end(uc_runtime_t *r)
{
    return ur_area_text(r)->end;
}

void            ur_free(void* v)
{
}

uc_pos_t        ur_strcpy(uc_runtime_t *r, uc_pos_t dst, uc_pos_t src)
{
    assert(0);
    return 0;
}

uc_pos_t        ur_stack_start(uc_runtime_t *r)
{
    return ur_area_stack(r)->begin;
}

uc_pos_t        ur_stack_end(uc_runtime_t *r)
{
    return ur_area_stack(r)->end;
}

uc_pos_t        ur_symbol_addr(uc_runtime_t *r, const char *symname)
{
    Elf32_Sym *sym = elf32_sym_get_by_name((Elf32_Ehdr *)r->fdata, symname);

    return r->elf_load_addr + sym->st_value;
}

void*           uc_vir2phy(uc_pos_t t)
{
    return 0;
}

uc_pos_t        ur_string(uc_runtime_t *r, const char *str)
{
    int len = strlen(str);
    uc_pos_t p = ur_malloc(r, len + 1);

    if (!p)
        return 0;

    uc_mem_write(r->uc, p, str, len + 1);

    return p;
}

struct uc_hook_func*    ur_hook_func_find(uc_runtime_t *r, const char *name)
{
    int i;
    struct uc_hook_func *f;

    for (i = 0, f= r->hooktab.list; i < r->hooktab.counts; i++, f = f->node.next) {
        if (!strcmp(f->name, name))
            return f;
    }

    return NULL;
}

struct uc_hook_func*    ur_hook_func_find_by_addr(uc_runtime_t *r, uint64_t addr)
{
    int i;
    struct uc_hook_func *f;

    for (i = 0, f= r->hooktab.list; i < r->hooktab.counts; i++, f = f->node.next) {
        if (f->address == addr)
            return f;
    }

    return NULL;
}

struct uc_hook_func*    ur_relocate_func(uc_runtime_t *t, const char *name, uint32_t offset)
{
    struct uc_hook_func *f;
    if (f = ur_hook_func_find(t, name))
        return f;

    uint32_t v = (uint32_t)ur__alloc(t, ur_area_text(t), 4, 4);
    if (!v) {
        assert(0);
    }

    f = calloc(1, sizeof (f[0]) + strlen(name));
    strcpy(f->name, name);
    f->address = v;
    mlist_add(t->hooktab, f, node);

    uc_mem_write(t->uc, offset, &v, sizeof (v));

    printf("hook func = %s\n", name);
    return f;
}

void            ur_elf_do_rel_sym_fix(uc_runtime_t *t, Elf32_Rel *rel)
{
    Elf32_Shdr *dynsymsh = elf32_shdr_get((Elf32_Ehdr *)t->fdata, SHT_DYNSYM);

    uint32_t rel_offset;
    int type, symind;
    Elf32_Sym *sym;
    const char *name;

    type = ELF32_R_TYPE(rel->r_info);
    symind = ELF32_R_SYM(rel->r_info);
    sym = ((Elf32_Sym *)(t->fdata + dynsymsh->sh_offset)) + symind;

    if (ELF32_ST_TYPE(sym->st_info) == STT_OBJECT) {
        uc_mem_read(t->uc, t->elf_load_addr + rel->r_offset,  (void *)&rel_offset, sizeof (rel_offset));
        rel_offset += (uint32_t)t->elf_load_addr;
        uc_mem_write(t->uc, t->elf_load_addr + rel->r_offset, (void *)&rel_offset, sizeof (rel_offset));
    }
    else if (ELF32_ST_TYPE(sym->st_info) == STT_FUNC) {
        if (sym->st_value) return;
        name = elf32_sym_name((Elf32_Ehdr *)t->fdata, sym);

        if (name && name[0])
            ur_relocate_func(t, name, (uint32_t)(rel->r_offset + t->elf_load_addr));
    }
}

static void            ur_elf_addr_fix(uc_runtime_t *t)
{
    Elf32_Shdr *dynsymsh = elf32_shdr_get((Elf32_Ehdr *)t->fdata, SHT_DYNSYM);
    Elf32_Shdr *sh = elf32_shdr_get_by_name((Elf32_Ehdr *)t->fdata, ".rel.dyn");
    Elf32_Rel *rel;
    int i, count;

    if (sh->sh_type != SHT_REL) {
        assert(0);
    }

    count = sh->sh_size / sh->sh_entsize;
    for (i = 0; i < count; i++) {
        rel = ((Elf32_Rel *)(t->fdata + sh->sh_offset)) + i;
        ur_elf_do_rel_sym_fix(t, rel);
    }

    sh = elf32_shdr_get_by_name((Elf32_Ehdr *)t->fdata, ".rel.plt");
    if (sh->sh_type != SHT_REL) {
        assert(0);
    }

    count = sh->sh_size / sh->sh_entsize;
    for (i = 0; i < count; i++) {
        rel = ((Elf32_Rel *)(t->fdata + sh->sh_offset)) + i;
        ur_elf_do_rel_sym_fix(t, rel);
    }
}

int             uc_on_hook_func(uc_runtime_t *t, uint64_t addr)
{
    return 0;
}

struct uc_hook_func*    ur_alloc_func(uc_runtime_t *t, const char *name, void (* cb)(void *user_data), void *user_data)
{
    struct uc_hook_func *f;
    if (f = ur_hook_func_find(t, name)) 
        return f;

    f = calloc(1, sizeof (f[0]) + strlen(name));
    if (!f)
        return NULL;

    strcpy(f->name, name);
    f->address = ur__alloc(t, ur_area_text(t), 4, 4);
    if (!f->address) {
        free(f);
        return NULL;
    }
    f->cb = cb;
    f->user_data = user_data;

    mlist_add(t->hooktab, f, node);

    return f;
}
