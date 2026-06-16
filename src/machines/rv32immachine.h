#ifndef RV32IMMACHINE_H
#define RV32IMMACHINE_H

#include "core/machine.h"

#include <QHash>
#include <QSet>
#include <QString>
#include <QMap>
#include <cstdint>

class RV32IMMachine : public Machine {
public:
  //////////////////////////////////////////////////
  // Memory layout constants (RARS-compatible)
  //////////////////////////////////////////////////
  static constexpr int TEXT_BASE  = 0x00400000;
  static constexpr int DATA_BASE  = 0x10040000;
  static constexpr int SP_INIT    = 0x7FFFFFFC;

    explicit RV32IMMachine(QObject *parent = 0);
    ~RV32IMMachine();

    //////////////////////////////////////////////////
    // Simulation step
    //////////////////////////////////////////////////
    /// RISC-V uses 32-bit fixed-length instructions,
    /// so the fetch/decode/execute pipeline differs
    /// entirely from the 8-bit variable-length system.
    void step() override;
    void fetchInstruction() override;
    void decodeInstruction() override;
    void executeInstruction() override;

    //////////////////////////////////////////////////
    // Assembler
    //////////////////////////////////////////////////
    /// Custom assembler handling RISC-V pseudo-instructions
    /// and the RV32IM instruction encoding.
    void assemble(QString sourceCode) override;

    //////////////////////////////////////////////////
    // Instruction strings
    //////////////////////////////////////////////////
    void updateInstructionStrings() override;
    QString generateInstructionString(int address, int &argumentsSize) override;
    QString generateArgumentsString(int address, Instruction *instruction, AddressingMode::AddressingModeCode addressingModeCode, int &argumentsSize) override;

    //////////////////////////////////////////////////
    // State management
    //////////////////////////////////////////////////
    void clear() override;
    void clearAfterBuild() override;
    void generateDescriptions() override;

    //////////////////////////////////////////////////
    // Memory access (sparse storage)
    //////////////////////////////////////////////////
    /// Memory size is 2^32, but only non-zero bytes
    /// are stored in the sparse hash.
    int getMemorySize() const override;
    int getMemoryValue(int address) const override;
    void setMemoryValue(int address, int value) override;
    bool hasByteChanged(int address) override;
    void clearMemory() override;

    //////////////////////////////////////////////////
    // Instruction strings (hash-based)
    //////////////////////////////////////////////////
    QString getInstructionString(int address) override;
    void setInstructionString(int address, QString value) override;
    void clearInstructionStrings() override;

    //////////////////////////////////////////////////
    // Address validation
    //////////////////////////////////////////////////
    bool isValidAddress(QString addressString) override;
    bool isValidOrg(QString offsetString) override;

    //////////////////////////////////////////////////
    // RISC-V memory read/write (byte/half/word)
    //////////////////////////////////////////////////
    int memoryReadByte(int address);
    int memoryReadHalf(int address);
    int memoryReadWord(int address);
    void memoryWriteByte(int address, int value);
    void memoryWriteHalf(int address, int value);
    void memoryWriteWord(int address, int value);

    //////////////////////////////////////////////////
    // Register access by name (ABI or numeric)
    //////////////////////////////////////////////////
    int getRegisterValueByName(const QString &name);
    void setRegisterValueByName(const QString &name, int value);
    bool hasRegister(QString registerName) const override;

    //////////////////////////////////////////////////
    // Override register access (reads from regFile[])
    //////////////////////////////////////////////////
    /// These override the virtual base class methods.
    /// Direct calls on RV32IMMachine will use these.
    int getRegisterValue(int id, bool signedData = false) const override;
    int getRegisterValue(QString registerName) const override;
    void setRegisterValue(int id, int value) override;
    void setRegisterValue(QString registerName, int value) override;

    //////////////////////////////////////////////////
    // Override PC access (reads from pcValue_)
    //////////////////////////////////////////////////
    int getPCValue() const override;
    void setPCValue(int value) override;
    void incrementPCValue(int units = 1) override;

    //////////////////////////////////////////////////
    // Override breakpoint access
    //////////////////////////////////////////////////
    int getBreakpoint() const override;
    void setBreakpoint(int value) override;

    //////////////////////////////////////////////////
    // Override label/source-line mapping
    //////////////////////////////////////////////////
    QString getAddressCorrespondingLabel(int address) override;
    int getLabelAddress(const QString& label) const override;
    QStringList getAllLabels() const override;
    int getAddressCorrespondingSourceLine(int address) override;
    int getSourceLineCorrespondingAddress(int line) override;
    int getPCCorrespondingSourceLine() override;

protected:
    //////////////////////////////////////////////////
    // Sparse memory storage
    //////////////////////////////////////////////////
    /// Only non-zero bytes are stored; reads to
    /// absent keys return 0.
    QHash<int, uint8_t> sparseMemory_;

    /// Tracks addresses modified since last check
    QSet<int> changedAddresses_;

    //////////////////////////////////////////////////
    // Program counter (separate from Machine::PC)
    //////////////////////////////////////////////////
    /// Actual PC value used by the RISC-V simulator.
    /// Machine::PC is kept in sync for base class compat.
    int pcValue_;

    /// PC of the currently executing instruction.
    /// Saved at fetch time, before PC is incremented.
    int currentPC_;

    //////////////////////////////////////////////////
    // Register file
    //////////////////////////////////////////////////
    /// 32 general-purpose registers (x0 hardwired to 0)
    int regFile[32];
    /// ABI name -> register index (e.g. "ra" -> 1, "sp" -> 2)
    static QMap<QString, int> abiNameToIndex_;
    /// Register index -> ABI name (e.g. 1 -> "ra", 2 -> "sp")
    static QMap<int, QString> indexToAbiName_;

    //////////////////////////////////////////////////
    // Fetched instruction (32-bit)
    //////////////////////////////////////////////////
    /// The full 32-bit instruction word fetched from
    /// memory. Stored separately because the base
    /// class fetchedValue is only 8-bit capable.
    uint32_t fetchedInstruction_;

    //////////////////////////////////////////////////
    // Decoded instruction fields
    //////////////////////////////////////////////////
    /// Fields extracted during decode from the 32-bit
    /// instruction word. Used by executeInstruction().
    int decodedOpcode_;
    int decodedRd_;
    int decodedRs1_;
    int decodedRs2_;
    int decodedFunct3_;
    int decodedFunct7_;
    int decodedImm_;

    //////////////////////////////////////////////////
    // Instruction strings (hash-based for sparse use)
    //////////////////////////////////////////////////
    QHash<int, QString> instructionStrings_;

    //////////////////////////////////////////////////
    // Assembler data (sparse, like runtime memory)
    //////////////////////////////////////////////////
    QHash<int, uint8_t> assemblerMemory_;
    QHash<QString, int> assemblerLabels_;
    QMap<QString, int> equates_;  // .eqv NAME value mappings

    //////////////////////////////////////////////////
    // Assembler status
    //////////////////////////////////////////////////
    bool buildSuccessfulRV32_;
    int firstErrorLineRV32_;
  //////////////////////////////////////////////////
  // Section tracking
  //////////////////////////////////////////////////
  int textCurrentAddr_;
  int dataCurrentAddr_;

    //////////////////////////////////////////////////
    // Helper methods
    //////////////////////////////////////////////////
    /// Sign-extend a value from bitWidth bits to 32 bits
    static int signExtend(int value, int bitWidth);
    /// Resolve a register name (ABI or x0-x31) to its
    /// index. Returns -1 if the name is not recognized.
    int resolveRegisterIndex(const QString &name) const;
    /// Resolve a register index to its ABI name if
    /// available, otherwise returns "xN" form.
    QString resolveRegisterName(int index) const;
    /// Write a value to regFile, enforcing x0 == 0
    void writeRegister(int index, int value);
};

#endif // RV32IMMACHINE_H
