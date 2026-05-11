#include "riscv_register_file.h"

namespace riscv {

RiscVRegisterFile::RiscVRegisterFile(QObject *parent)
    : QObject(parent)
    , gprs_(NUM_GPRS, 0)
    , pc_(0)
    , cycleCount_(0)
{
    // x0 é sempre zero - garantido pela inicialização acima
    // e pela lógica em writeRegister que ignora escritas em x0
    
    // Inicializa CSRs básicos com valores padrão
    csrs_[static_cast<uint16_t>(CSRId::mvendorid)] = 0;  // Sem vendor ID específico
    csrs_[static_cast<uint16_t>(CSRId::marchid)]   = 0;  // Architecture ID genérico
    csrs_[static_cast<uint16_t>(CSRId::mimpid)]    = 0;  // Sem implementation ID
    csrs_[static_cast<uint16_t>(CSRId::mhartid)]   = 0;  // Hart ID 0 (single-core)
    
    // mstatus: bits básicos (MIE desabilitado por padrão)
    csrs_[static_cast<uint16_t>(CSRId::mstatus)]   = 0x00000080;  // MPRV=0, MPP=0
    
    // mtvec: vetor de trap (base=0, mode=direct)
    csrs_[static_cast<uint16_t>(CSRId::mtvec)]     = 0;
    
    // Contadores zerados
    csrs_[static_cast<uint16_t>(CSRId::mcycle)]    = 0;
    csrs_[static_cast<uint16_t>(CSRId::minstret)]  = 0;
}

RV32Reg RiscVRegisterFile::readRegister(int regId) const {
    if (!isValidRegId(regId)) {
        return 0;
    }
    
    // x0 sempre retorna 0, mesmo se tentarem escrever
    if (regId == static_cast<int>(RegId::x0)) {
        return 0;
    }
    
    return gprs_[regId];
}

RV32Reg RiscVRegisterFile::readRegisterByName(const QString &name) const {
    int regId = getRegIdByName(name);
    if (regId < 0) {
        return 0;
    }
    return readRegister(regId);
}

bool RiscVRegisterFile::writeRegister(int regId, RV32Reg value) {
    // x0 é hardwired para zero - qualquer escrita é ignorada
    if (regId == static_cast<int>(RegId::x0)) {
        return false;
    }
    
    if (!isValidRegId(regId)) {
        return false;
    }
    
    RV32Reg oldValue = gprs_[regId];
    RV32Reg newValue = maskTo32Bits(value);
    
    gprs_[regId] = newValue;
    
    // Emite signal para debugging/hooks
    emit registerWritten(regId, oldValue, newValue);
    
    return true;
}

bool RiscVRegisterFile::writeRegisterByName(const QString &name, RV32Reg value) {
    int regId = getRegIdByName(name);
    if (regId < 0) {
        return false;
    }
    return writeRegister(regId, value);
}

void RiscVRegisterFile::setPC(RV32Addr address) {
    RV32Addr oldPC = pc_;
    
    // Alinha endereço a 4 bytes (instruções RV32I têm 4 bytes)
    // Nota: isso é uma simplificação - na especificação real,
    // acessos não alinhados causam exceção
    pc_ = address & 0xFFFFFFFCu;
    
    if (oldPC != pc_) {
        emit pcChanged(oldPC, pc_);
    }
}

void RiscVRegisterFile::incrementPC(RV32Addr increment) {
    setPC(pc_ + increment);
}

RV32Reg RiscVRegisterFile::readCSR(CSRId csrId) const {
    return readCSRById(static_cast<uint16_t>(csrId));
}

RV32Reg RiscVRegisterFile::readCSRById(uint16_t csrId) const {
    auto it = csrs_.find(csrId);
    if (it != csrs_.end()) {
        // Atualiza mcycle antes de retornar
        if (csrId == static_cast<uint16_t>(CSRId::mcycle) ||
            csrId == static_cast<uint16_t>(CSRId::cycle)) {
            return maskTo32Bits(cycleCount_ & 0xFFFFFFFFu);
        }
        return *it;
    }
    
    // CSR não implementado retorna 0
    return 0;
}

bool RiscVRegisterFile::writeCSR(CSRId csrId, RV32Reg value) {
    return writeCSRById(static_cast<uint16_t>(csrId), value);
}

bool RiscVRegisterFile::writeCSRById(uint16_t csrId, RV32Reg value) {
    // Verifica se CSR é read-only
    // CSRs com bit 11 setado são read-only
    if (csrId & 0x0800) {
        return false;
    }
    
    // Alguns CSRs específicos são read-only ou têm comportamento especial
    switch (static_cast<CSRId>(csrId)) {
        case CSRId::mvendorid:
        case CSRId::marchid:
        case CSRId::mimpid:
        case CSRId::mhartid:
            // Information registers são read-only
            return false;
            
        case CSRId::mcycle:
            // Permite escrever em mcycle para reset/ajuste
            break;
            
        default:
            break;
    }
    
    RV32Reg maskedValue = maskTo32Bits(value);
    csrs_[csrId] = maskedValue;
    
    emit csrWritten(csrId, maskedValue);
    
    return true;
}

QString RiscVRegisterFile::getRegName(int regId) {
    if (!isValidRegId(regId)) {
        return "invalid";
    }
    return QString(REG_ABI_NAMES[regId]);
}

int RiscVRegisterFile::getRegIdByName(const QString &name) {
    QString lowerName = name.toLower();
    
    // Tenta encontrar por nome ABI
    for (int i = 0; i < NUM_GPRS; ++i) {
        if (lowerName == QString(REG_ABI_NAMES[i])) {
            return i;
        }
    }
    
    // Tenta encontrar por formato numérico (ex: "x0", "x31")
    if (lowerName.startsWith('x')) {
        bool ok = false;
        int regNum = lowerName.mid(1).toInt(&ok);
        if (ok && isValidRegId(regNum)) {
            return regNum;
        }
    }
    
    // Tenta encontrar por número puro (ex: "0", "31")
    bool ok = false;
    int regNum = name.toInt(&ok);
    if (ok && isValidRegId(regNum)) {
        return regNum;
    }
    
    return -1;
}

void RiscVRegisterFile::reset() {
    // Zera todos os GPRs (x0 já é zero por definição)
    for (int i = 1; i < NUM_GPRS; ++i) {
        RV32Reg oldValue = gprs_[i];
        gprs_[i] = 0;
        if (oldValue != 0) {
            emit registerWritten(i, oldValue, 0);
        }
    }
    
    // Reseta PC
    resetPC(0);
    
    // Reseta CSRs modificáveis
    csrs_[static_cast<uint16_t>(CSRId::mstatus)]  = 0x00000080;
    csrs_[static_cast<uint16_t>(CSRId::mie)]      = 0;
    csrs_[static_cast<uint16_t>(CSRId::mtvec)]    = 0;
    csrs_[static_cast<uint16_t>(CSRId::mscratch)] = 0;
    csrs_[static_cast<uint16_t>(CSRId::mepc)]     = 0;
    csrs_[static_cast<uint16_t>(CSRId::mcause)]   = 0;
    csrs_[static_cast<uint16_t>(CSRId::mtval)]    = 0;
    csrs_[static_cast<uint16_t>(CSRId::mip)]      = 0;
    
    cycleCount_ = 0;
}

void RiscVRegisterFile::resetPC(RV32Addr startAddress) {
    setPC(startAddress);
}

bool RiscVRegisterFile::isReadOnlyCSR(CSRId csrId) const {
    uint16_t id = static_cast<uint16_t>(csrId);
    
    // Bits [11:9] indicam privacidade e writability
    // Se bit 11 = 1, CSR é read-only
    if (id & 0x0800) {
        return true;
    }
    
    // Information registers são sempre read-only
    switch (csrId) {
        case CSRId::mvendorid:
        case CSRId::marchid:
        case CSRId::mimpid:
        case CSRId::mhartid:
            return true;
        default:
            return false;
    }
}

} // namespace riscv
