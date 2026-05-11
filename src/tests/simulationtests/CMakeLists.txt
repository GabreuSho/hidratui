add_subdirectory(Ahmes)
add_subdirectory(Neander)
add_subdirectory(Ramses)

# Testes RISC-V (se habilitado)
if(ENABLE_RISCV)
    add_subdirectory(RiscV)
endif()