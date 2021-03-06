// license:GPL-2.0+
// copyright-holders:Segher Boessenkool,Ryan Holtz
/*****************************************************************************

    SunPlus µ'nSP emulator

    Copyright 2008-2017  Segher Boessenkool  <segher@kernel.crashing.org>
    Licensed under the terms of the GNU GPL, version 2
    http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

    Ported to MAME framework by Ryan Holtz

*****************************************************************************/

#ifndef MAME_CPU_UNSP_UNSP_H
#define MAME_CPU_UNSP_UNSP_H

#pragma once

#include "cpu/drcfe.h"
#include "cpu/drcuml.h"
#include "cpu/drcumlsh.h"

/***************************************************************************
    CONSTANTS
***************************************************************************/

/* map variables */
#define MAPVAR_PC				M0
#define MAPVAR_CYCLES			M1

#define UNSP_STRICT_VERIFY		0x0001          /* verify all instructions */

#define SINGLE_INSTRUCTION_MODE	(0)

#define ENABLE_UNSP_DRC			(1)

#define UNSP_LOG_OPCODES		(0)
#define UNSP_LOG_REGS			(0)

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class unsp_frontend;

// ======================> unsp_device

enum
{
	UNSP_SP = 1,
	UNSP_R1,
	UNSP_R2,
	UNSP_R3,
	UNSP_R4,
	UNSP_BP,
	UNSP_SR,
	UNSP_PC,

	UNSP_GPR_COUNT = UNSP_PC,

	UNSP_IRQ_EN,
	UNSP_FIQ_EN,
	UNSP_IRQ,
	UNSP_FIQ,
#if UNSP_LOG_OPCODES
	UNSP_SB,
	UNSP_LOG_OPS
#else
	UNSP_SB
#endif
};

enum
{
	UNSP_FIQ_LINE = 0,
	UNSP_IRQ0_LINE,
	UNSP_IRQ1_LINE,
	UNSP_IRQ2_LINE,
	UNSP_IRQ3_LINE,
	UNSP_IRQ4_LINE,
	UNSP_IRQ5_LINE,
	UNSP_IRQ6_LINE,
	UNSP_IRQ7_LINE,
	UNSP_BRK_LINE,

	UNSP_NUM_LINES
};

class unsp_device : public cpu_device
{
	friend class unsp_frontend;

public:
	// construction/destruction
	unsp_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	uint8_t get_csb();

	inline void ccfunc_unimplemented();
	void invalidate_cache();
#if UNSP_LOG_REGS
	void log_regs();
#endif

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_stop() override;

	// device_execute_interface overrides
	virtual uint32_t execute_min_cycles() const override { return 5; }
	virtual uint32_t execute_max_cycles() const override { return 5; }
	virtual uint32_t execute_input_lines() const override { return 0; }
	virtual void execute_run() override;
	virtual void execute_set_input(int inputnum, int state) override;

	// device_memory_interface overrides
	virtual space_config_vector memory_space_config() const override;

	// device_state_interface overrides
	virtual void state_import(const device_state_entry &entry) override;
	virtual void state_export(const device_state_entry &entry) override;
	virtual void state_string_export(const device_state_entry &entry, std::string &str) const override;

	// device_disasm_interface overrides
	virtual std::unique_ptr<util::disasm_interface> create_disassembler() override;

private:
	// compilation boundaries -- how far back/forward does the analysis extend?
	enum : uint32_t
	{
		COMPILE_BACKWARDS_BYTES     = 128,
		COMPILE_FORWARDS_BYTES      = 512,
		COMPILE_MAX_INSTRUCTIONS    = (COMPILE_BACKWARDS_BYTES / 4) + (COMPILE_FORWARDS_BYTES / 4),
		COMPILE_MAX_SEQUENCE        = 64
	};

	// exit codes
	enum : int
	{
		EXECUTE_OUT_OF_CYCLES       = 0,
		EXECUTE_MISSING_CODE        = 1,
		EXECUTE_UNMAPPED_CODE       = 2,
		EXECUTE_RESET_CACHE         = 3
	};

	enum : uint32_t
	{
		REG_SP = 0,
		REG_R1,
		REG_R2,
		REG_R3,
		REG_R4,
		REG_BP,
		REG_SR,
		REG_PC
	};

	void add_lpc(const int32_t offset);

	inline void execute_one(const uint16_t op);

	struct internal_unsp_state
	{
		uint32_t m_r[8];
		uint32_t m_enable_irq;
		uint32_t m_enable_fiq;
		uint32_t m_irq;
		uint32_t m_fiq;
		uint32_t m_curirq;
		uint32_t m_sirq;
		uint32_t m_sb;
		uint32_t m_saved_sb[3];

		uint32_t m_arg0;
		uint32_t m_arg1;
		uint32_t m_arg2;
		uint32_t m_jmpdest;

		int m_icount;
	};

	address_space_config m_program_config;
	address_space *m_program;
	std::function<u16 (offs_t)> m_pr16;
	std::function<const void * (offs_t)> m_prptr;

	/* core state */
	internal_unsp_state *m_core;

	uint32_t m_debugger_temp;
#if UNSP_LOG_OPCODES
	uint32_t m_log_ops;
#endif

	void unimplemented_opcode(uint16_t op);
	inline uint16_t read16(uint32_t address);
	inline void write16(uint32_t address, uint16_t data);
	inline void update_nz(uint32_t value);
	inline void update_nzsc(uint32_t value, uint16_t r0, uint16_t r1);
	inline void push(uint32_t value, uint32_t *reg);
	inline uint16_t pop(uint32_t *reg);
	inline void trigger_fiq();
	inline void trigger_irq(int line);
	inline void check_irqs();

	drc_cache m_cache;
	std::unique_ptr<drcuml_state> m_drcuml;
	std::unique_ptr<unsp_frontend> m_drcfe;
	uint32_t m_drcoptions;
	uint8_t m_cache_dirty;

	uml::parameter   m_regmap[16];
	uml::code_handle *m_entry;
	uml::code_handle *m_nocode;
	uml::code_handle *m_out_of_cycles;
	uml::code_handle *m_check_interrupts;
	uml::code_handle *m_trigger_fiq;
	uml::code_handle *m_trigger_irq;

	uml::code_handle *m_mem_read;
	uml::code_handle *m_mem_write;

	bool m_enable_drc;

	/* internal compiler state */
	struct compiler_state
	{
		compiler_state(compiler_state const &) = delete;
		compiler_state &operator=(compiler_state const &) = delete;

		uint32_t m_cycles;          /* accumulated cycles */
		uml::code_label m_labelnum; /* index for local labels */
	};

	void execute_run_drc();
	void flush_drc_cache();
	void code_flush_cache();
	void code_compile_block(offs_t pc);
	void load_fast_iregs(drcuml_block &block);
	void save_fast_iregs(drcuml_block &block);

	void static_generate_entry_point();
	void static_generate_nocode_handler();
	void static_generate_out_of_cycles();
	void static_generate_check_interrupts();
	void static_generate_trigger_fiq();
	void static_generate_trigger_irq();
	void static_generate_memory_accessor(bool iswrite, const char *name, uml::code_handle *&handleptr);

	void generate_branch(drcuml_block &block, compiler_state &compiler, const opcode_desc *desc);
	void generate_update_cycles(drcuml_block &block, compiler_state &compiler, uml::parameter param);
	void generate_checksum_block(drcuml_block &block, compiler_state &compiler, const opcode_desc *seqhead, const opcode_desc *seqlast);
	void generate_sequence_instruction(drcuml_block &block, compiler_state &compiler, const opcode_desc *desc);
	void generate_push(drcuml_block &block, uint32_t sp);
	void generate_pop(drcuml_block &block, uint32_t sp);
	void generate_add_lpc(drcuml_block &block, int32_t offset);
	void generate_update_nzsc(drcuml_block &block);
	void generate_update_nz(drcuml_block &block);
	void log_add_disasm_comment(drcuml_block &block, uint32_t pc, uint32_t op);
	bool generate_opcode(drcuml_block &block, compiler_state &compiler, const opcode_desc *desc);

#if UNSP_LOG_REGS
	FILE *m_log_file;
#endif
};


DECLARE_DEVICE_TYPE(UNSP, unsp_device)

#endif // MAME_CPU_UNSP_UNSP_H
