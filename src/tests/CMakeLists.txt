# Required for testing

add_subdirectory(baseconversortest)
add_subdirectory(pointconversortest)
add_subdirectory(simulationtests)

# Testes RISC-V (opcional, se habilitado no build principal)
if(ENABLE_RISCV)
    # Os testes unitários principais estão em tests/riscv/
    # Este subdiretório é para testes de integração com o framework existente
endif()