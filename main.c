#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#include "m3.h"
#include "m3_api_wasi.h"
#include "m3_api_libc.h"
#include "m3_env.h"


IM3Module load (IM3Runtime runtime, const char* fn) {
    u8* wasm = NULL;
    u32 fsize = 0;
    FILE* f = fopen (fn, "rb");
    fseek (f, 0, SEEK_END);
    fsize = ftell(f);
    fseek (f, 0, SEEK_SET);
    wasm = (u8*) malloc(fsize);
    fread (wasm, 1, fsize, f);
    fclose (f);
    IM3Module module;
    M3Result result = m3_ParseModule (runtime->environment, &module, wasm, fsize);
    result = m3_LoadModule (runtime, module);
    result = m3_LinkSpecTest (runtime->modules);
    m3_LinkWASI (runtime->modules);
    return module;
}

void print_alloc_mem(IM3Runtime runtime) {
    printf("Runtime allocated memory: %ld bytes\n", runtime->memory.mallocated->length);
}

int main() {
    IM3Environment env = m3_NewEnvironment();
    IM3Runtime runtime = m3_NewRuntime(env, 1024*8, NULL);
    if(runtime == NULL) {
        printf("Couldn't init runtime\n");
        return 1;
    };

    // Load libxml2:
    IM3Module module;
    module = load(runtime, "libxml2.wasm");
    printf("WASM module loaded\n");
    print_alloc_mem(runtime);

    clock_t begin = clock();

    // Load XSD file:
    u8* xsd_buf = NULL;
    u32 xsd_buf_size = 0;
    FILE* xsd_file = fopen("input.xsd", "rb");
    fseek (xsd_file, 0, SEEK_END);
    xsd_buf_size = ftell(xsd_file);
    fseek (xsd_file, 0, SEEK_SET);
    xsd_buf = (u8*) malloc(xsd_buf_size);
    fread (xsd_buf, 1, xsd_buf_size, xsd_file);
    fclose(xsd_file);
    
    printf("XSD file loaded (%d bytes)\n", xsd_buf_size);

    IM3Function allocate_fn;
    m3_FindFunction (&allocate_fn, runtime, "wasm_allocate");
    m3stack_t allocate_stack = (m3stack_t)(runtime->stack);
    m3stack_t allocate_stack_0 = &allocate_stack[0];
    *(u32*)(allocate_stack_0) = xsd_buf_size;
    m3StackCheckInit();
    Call(allocate_fn->compiled, allocate_stack, runtime->memory.mallocated, d_m3OpDefaultArgs);
    int xsd_buf_ptr = *(u32*)(allocate_stack);

    printf("Allocated %d bytes, xsd_buf_ptr = %d\n", xsd_buf_ptr, xsd_buf_size);

    void *dest = ((u8*)(runtime->memory.mallocated) + (u32)(xsd_buf_ptr) + 24);
    memcpy(dest, xsd_buf, xsd_buf_size);
    print_alloc_mem(runtime);


    // Initialize a new XML schema parser:
    IM3Function new_schema_parser_fn;
    m3_FindFunction (&new_schema_parser_fn, runtime, "wasm_new_schema_parser2");
    m3stack_t new_schema_stack = (m3stack_t)(runtime->stack);
    m3stack_t new_schema_stack_0 = &new_schema_stack[0];
    m3stack_t new_schema_stack_1 = &new_schema_stack[1];
    *(u32*)(new_schema_stack_0) = xsd_buf_ptr;
    *(u32*)(new_schema_stack_1) = xsd_buf_size;
    m3StackCheckInit();
    Call(new_schema_parser_fn->compiled, new_schema_stack, runtime->memory.mallocated, d_m3OpDefaultArgs);
    int schema_ptr = *(u32*)(allocate_stack);

    // Load input XML file:
    u8* xml_buf = NULL;
    u32 xml_buf_size = 0;
    FILE* xml_file = fopen ("input.xml", "rb");
    fseek (xml_file, 0, SEEK_END);
    xml_buf_size = ftell(xml_file);
    fseek (xml_file, 0, SEEK_SET);
    xml_buf = (u8*) malloc(xml_buf_size);
    fread (xml_buf, 1, xml_buf_size, xml_file);
    fclose(xml_file);

    printf("XML file loaded (%d bytes)\n", xml_buf_size);

    m3stack_t allocate_stack2 = (m3stack_t)(runtime->stack);
    m3stack_t allocate_stack2_0 = &allocate_stack2[0];
    *(u32*)(allocate_stack2_0) = xml_buf_size;
    m3StackCheckInit();
    Call(allocate_fn->compiled, allocate_stack2, runtime->memory.mallocated, d_m3OpDefaultArgs);
    int xml_buf_ptr = *(u32*)(allocate_stack2);
    printf("Allocated %d bytes, xml_buf_ptr = %d\n", xml_buf_size, xml_buf_ptr);

    dest = ((u8*)(runtime->memory.mallocated) + (u32)(xml_buf_ptr) + 24);
    memcpy(dest, xml_buf, xml_buf_size);

    IM3Function validate_fn;
    m3_FindFunction (&validate_fn, runtime, "wasm_validate_xml");
    m3stack_t validate_stack = (m3stack_t)(runtime->stack);
    m3stack_t validate_stack_0 = &validate_stack[0];
    m3stack_t validate_stack_1 = &validate_stack[1];
    m3stack_t validate_stack_2 = &validate_stack[2];
    *(u32*)(validate_stack_0) = (u32)xml_buf_ptr;
    *(u32*)(validate_stack_1) = (u32)xml_buf_size;
    *(u32*)(validate_stack_2) = (u32)schema_ptr;
    m3StackCheckInit();
    Call(validate_fn->compiled, validate_stack, runtime->memory.mallocated, d_m3OpDefaultArgs);
    int result = *(u32*)(validate_stack);
    printf("wasm_validate_xml = %d\n", result);

    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("spent: %2f\n", time_spent);

    m3_FreeRuntime (runtime);
    m3_FreeEnvironment (env);
}
