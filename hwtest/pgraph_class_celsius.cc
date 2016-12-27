/*
 * Copyright (C) 2016 Marcin Kościelnicki <koriakin@0x04.net>
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "pgraph.h"
#include "pgraph_mthd.h"
#include "pgraph_class.h"
#include "nva.h"

namespace hwtest {
namespace pgraph {

class MthdWarning : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return val < 3;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			insrt(exp.notify, 24, 1, !!(val & 3));
			insrt(exp.notify, 25, 1, (val & 3) == 2);
			insrt(exp.notify, 28, 3, 0);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdState : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	void emulate_mthd() override {
		insrt(exp.intr, 4, 1, 1);
		exp.fifo_enable = 0;
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdDmaVtx : public SingleMthdTest {
	bool takes_dma() override { return true; }
	void emulate_mthd() override {
		uint32_t rval = val & 0xffff;
		int dcls = extr(pobj[0], 0, 12);
		if (dcls == 0x30)
			rval = 0;
		bool bad = false;
		if (dcls != 0x30 && dcls != 0x3d && dcls != 2)
			bad = true;
		if (bad && extr(exp.debug[3], 23, 1))
			nv04_pgraph_blowup(&exp, 2);
		bool prot_err = false;
		if (dcls != 0x30) {
			if (extr(pobj[1], 0, 8) != 0xff)
				prot_err = true;
			if (extr(pobj[0], 20, 8))
				prot_err = true;
			if (!extr(pobj[0], 12, 2)) {
				exp.intr |= 0x400;
				exp.fifo_enable = 0;
			}
		}
#if 0
		// XXX: figure this out
		if (dcls == 0x30 && nv04_pgraph_is_nv15p(&chipset)) {
			exp.intr |= 0x400;
			exp.fifo_enable = 0;
		}
#endif
		if (prot_err)
			nv04_pgraph_blowup(&exp, 4);
		insrt(exp.celsius_dma, 0, 16, rval);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdDmaState : public SingleMthdTest {
	bool takes_dma() override { return true; }
	void emulate_mthd() override {
		uint32_t rval = val & 0xffff;
		int dcls = extr(pobj[0], 0, 12);
		if (dcls == 0x30)
			rval = 0;
		bool bad = false;
		if (dcls != 0x30 && dcls != 0x3d && dcls != 3)
			bad = true;
		if (bad && extr(exp.debug[3], 23, 1))
			nv04_pgraph_blowup(&exp, 2);
		insrt(exp.celsius_dma, 16, 16, rval);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusTexOffset : public SingleMthdTest {
	void adjust_orig_mthd() override {
		val &= ~0x7f;
		if (rnd() & 1) {
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return !(val & 0x7f);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.celsius_tex_offset[idx] = val;
		}
		insrt(exp.valid[1], idx ? 22 : 14, 1, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusTexFormat : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 3)
				insrt(val, 0, 2, 1);
			if (rnd() & 3)
				insrt(val, 3, 2, 1);
			if (rnd() & 3)
				insrt(val, 5, 2, 1);
			if (!(rnd() & 3))
				insrt(val, 7, 5, 0x19 + rnd() % 4);
			if (rnd() & 1)
				insrt(val, 20, 4, extr(val, 16, 4));
			if (rnd() & 3)
				insrt(val, 24, 3, 3);
			if (rnd() & 3)
				insrt(val, 28, 3, 3);
			if (rnd() & 1) {
				if (rnd() & 3)
					insrt(val, 2, 1, 0);
				if (rnd() & 3)
					insrt(val, 12, 4, 1);
			}
		}
	}
	bool is_valid_val() override {
		int mips = extr(val, 12, 4);
		int su = extr(val, 16, 4);
		int sv = extr(val, 20, 4);
		int fmt = extr(val, 7, 5);
		if (!extr(val, 0, 2) || extr(val, 0, 2) == 3)
			return false;
		if (extr(val, 2, 1)) {
			if (su != sv)
				return false;
			if (su >= 0xa && (fmt == 6 || fmt == 7 || fmt == 0xb || fmt == 0xe || fmt == 0xf))
				return false;
			if (su >= 0xb)
				return false;
		}
		if (!extr(val, 3, 2) || extr(val, 3, 2) == 3)
			return false;
		if (!extr(val, 5, 2) || extr(val, 5, 2) == 3)
			return false;
		if (fmt == 0xd)
			return false;
		if (fmt >= 0x1d)
			return false;
		if (mips > 0xc || mips == 0)
			return false;
		if (fmt >= 0x19) {
			if (extr(val, 3, 2) != 1)
				return false;
			if (cls != 0x99)
				return false;
		}
		if (fmt >= 0x10) {
			if (extr(val, 2, 1))
				return false;
			if (extr(val, 24, 3) != 3)
				return false;
			if (extr(val, 28, 3) != 3)
				return false;
			if (mips != 1)
				return false;
		}
		if (su > 0xb || sv > 0xb)
			return false;
		if (extr(val, 24, 3) < 1 || extr(val, 24, 3) > 5)
			return false;
		if (extr(val, 28, 3) < 1 || extr(val, 28, 3) > 5)
			return false;
		return true;
	}
	void emulate_mthd() override {
		uint32_t rval = val & 0xffffffd6;
		int mips = extr(val, 12, 4);
		int su = extr(val, 16, 4);
		int sv = extr(val, 20, 4);
		if (mips > su && mips > sv)
			mips = (su > sv ? su : sv) + 1;
		insrt(rval, 12, 4, mips);
		if (!extr(exp.nsource, 1, 1)) {
			exp.celsius_tex_format[idx] = rval | (exp.celsius_tex_format[idx] & 8);
		}
		insrt(exp.valid[1], idx ? 21 : 15, 1, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusTexControl : public SingleMthdTest {
	bool is_valid_val() override {
		if (extr(val, 31, 1))
			return false;
		if (extr(val, 5, 1))
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.celsius_tex_control[idx] = val & 0x7fffffff;
			insrt(exp.celsius_xf_misc_b, idx ? 14 : 0, 1, extr(val, 30, 1));
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusTexPitch : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= ~0xffff;
			if (!(rnd() & 3)) {
				val &= 0xe00f0000;
			}
			if (!(rnd() & 3))
				val = 0;
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
				val ^= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		return !(val & 0xffff) && !!(val & 0x3ff80000);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.celsius_tex_pitch[idx] = val & 0xffff0000;
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusTexUnk238 : public SingleMthdTest {
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.celsius_tex_unk238[idx] = val;
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusTexRect : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				val &= ~0xf000f800;
			}
			if (rnd() & 1) {
				if (rnd() & 1) {
					val &= ~0xffff;
				} else {
					val &= ~0xffff0000;
				}
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		if (extr(val, 16, 1))
			return false;
		if (!extr(val, 0, 16) || extr(val, 0, 16) >= 0x800)
			return false;
		if (!extr(val, 17, 15) || extr(val, 17, 15) >= 0x800)
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.celsius_tex_rect[idx] = val & 0x07ff07ff;
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusTexFilter : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 3)
				insrt(val, 13, 11, 0);
			if (rnd() & 3)
				insrt(val, 24, 4, 1);
			if (rnd() & 3)
				insrt(val, 28, 4, 1);
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		if (extr(val, 13, 11))
			return false;
		if (extr(val, 24, 4) < 1 || extr(val, 24, 4) > 6)
			return false;
		if (extr(val, 28, 4) < 1 || extr(val, 28, 4) > 2)
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.celsius_tex_filter[idx] = val & 0x77001fff;
		}
		insrt(exp.valid[1], idx ? 23 : 16, 1, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusTexPalette : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= ~0x3c;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		return !(val & 0x3e);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.celsius_tex_palette[idx] = val & 0xffffffc1;
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusRcInAlpha : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 16, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 24, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		for (int j = 0; j < 4; j++) {
			int reg = extr(val, 8 * j, 4);
			if (reg == 6 || reg == 7 || reg == 0xa || reg == 0xb || reg >= 0xe)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.celsius_rc_in[0][idx] = val;
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusRcInColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 16, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 24, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		for (int j = 0; j < 4; j++) {
			int reg = extr(val, 8 * j, 4);
			if (reg == 6 || reg == 7 || reg == 0xa || reg == 0xb || reg >= 0xe)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.celsius_rc_in[1][idx] = val;
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusRcFactor : public SingleMthdTest {
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.celsius_rc_factor[idx] = val;
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusRcOutAlpha : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 18, 14, 0);
			}
			if (rnd() & 1) {
				insrt(val, 28, 2, 1);
			}
			if (rnd() & 1) {
				insrt(val, 12, 2, 0);
			}
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 4, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		int op = extr(val, 15, 3);
		if (op == 5 || op == 7)
			return false;
		if (extr(val, 12, 2))
			return false;
		for (int j = 0; j < 3; j++) {
			int reg = extr(val, 4 * j, 4);
			if (reg != 0 && reg != 4 && reg != 5 && reg != 8 && reg != 9 && reg != 0xc && reg != 0xd)
				return false;
		}
		if (extr(val, 18, 14))
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.celsius_rc_out[0][idx] = val & 0x3cfff;
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusRcOutColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 18, 14, 0);
			}
			if (rnd() & 1) {
				insrt(val, 28, 2, 1);
			}
			if (rnd() & 1) {
				insrt(val, 12, 2, 0);
			}
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 4, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		int op = extr(val, 15, 3);
		if (op == 5 || op == 7)
			return false;
		if (!idx) {
			if (extr(val, 27, 3))
				return false;
		} else {
			int cnt = extr(val, 28, 2);
			if (cnt == 0 || cnt == 3)
				return false;
		}
		for (int j = 0; j < 3; j++) {
			int reg = extr(val, 4 * j, 4);
			if (reg != 0 && reg != 4 && reg != 5 && reg != 8 && reg != 9 && reg != 0xc && reg != 0xd)
				return false;
		}
		if (extr(val, 18, 9))
			return false;
		if (extr(val, 30, 2))
			return false;
		return true;
	}
	void emulate_mthd() override {
		uint32_t rval;
		if (!idx) {
			rval = val & 0x3ffff;
		} else {
			rval = val & 0x3803ffff;
		}
		if (!extr(exp.nsource, 1, 1)) {
			exp.celsius_rc_out[1][idx] = rval;
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdDmaClipid : public SingleMthdTest {
	bool takes_dma() override { return true; }
	void emulate_mthd() override {
		uint32_t offset_mask = pgraph_offset_mask(&chipset) | 0xf;
		uint32_t base = (pobj[2] & ~0xfff) | extr(pobj[0], 20, 12);
		uint32_t limit = pobj[1];
		uint32_t dcls = extr(pobj[0], 0, 12);
		exp.celsius_surf_limit_clipid = (limit & offset_mask) | (dcls != 0x30) << 31;
		exp.celsius_surf_base_clipid = base & offset_mask;
		bool bad = true;
		if (dcls == 0x30 || dcls == 0x3d)
			bad = false;
		if (dcls == 3 && (cls == 0x38 || cls == 0x88))
			bad = false;
		if (extr(exp.debug[3], 23, 1) && bad) {
			nv04_pgraph_blowup(&exp, 0x2);
		}
		bool prot_err = false;
		if (extr(base, 0, 6))
			prot_err = true;
		if (extr(pobj[0], 16, 2))
			prot_err = true;
		if (dcls == 0x30)
			prot_err = false;
		if (prot_err)
			nv04_pgraph_blowup(&exp, 4);
		insrt(exp.ctx_valid, 28, 1, dcls != 0x30 && !(bad && extr(exp.debug[3], 23, 1)));
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusRcFinal0 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x3f3f3f3f;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		if (val & 0xc0c0c0c0)
			return false;
		for (int j = 0; j < 4; j++) {
			int reg = extr(val, 8 * j, 4);
			if (reg == 6 || reg == 7 || reg == 0xa || reg == 0xb)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.celsius_rc_final[0] = val & 0x3f3f3f3f;
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusRcFinal1 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x3f3f3fe0;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		if (val & 0xc0c0c01f)
			return false;
		for (int j = 1; j < 4; j++) {
			int reg = extr(val, 8 * j, 4);
			if (reg == 6 || reg == 7 || reg == 0xa || reg == 0xb || reg >= 0xe)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.celsius_rc_final[1] = val & 0x3f3f3fe0;
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusConfig : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x31111101;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		if (val & 0xceeeeefe)
			return false;
		if (extr(val, 28, 2) && cls != 0x99)
			return false;
		if (extr(val, 8, 1) && !extr(val, 16, 1))
			return false;
		return true;
	}
	void emulate_mthd() override {
		insrt(exp.valid[1], 17, 1, 1);
		if (!extr(exp.nsource, 1, 1)) {
			insrt(exp.celsius_config_a, 25, 1, extr(val, 0, 1));
			insrt(exp.celsius_config_a, 23, 1, extr(val, 16, 1));
			insrt(exp.celsius_config_b, 2, 1, extr(val, 24, 1));
			insrt(exp.celsius_config_b, 6, 1, extr(val, 20, 1));
			insrt(exp.celsius_config_b, 10, 4, extr(val, 8, 4));
			if (nv04_pgraph_is_nv17p(&chipset))
				insrt(exp.celsius_config_b, 14, 2, extr(val, 28, 2));
			insrt(exp.celsius_raster, 29, 1, extr(val, 12, 1));
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusLightModel : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x00010007;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		return !(val & 0xfffefff8);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			insrt(exp.celsius_xf_misc_a, 18, 1, extr(val, 2, 1));
			insrt(exp.celsius_xf_misc_a, 19, 1, extr(val, 0, 1));
			insrt(exp.celsius_xf_misc_a, 20, 1, extr(val, 1, 1));
			insrt(exp.celsius_xf_misc_b, 28, 1, extr(val, 16, 1));
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusLightMaterial : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		if (val & ~0xf) {
			warn(1);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_xf_misc_a, 21, 4, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusFogMode : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
				if (rnd() & 1) {
					val |= (rnd() & 1 ? 0x800 : 0x2600);
				}
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		uint32_t rv = 0;
		if (val == 0x800) {
			rv = 1;
		} else if (val == 0x801) {
			rv = 3;
		} else if (val == 0x802) {
			rv = 5;
		} else if (val == 0x803) {
			rv = 7;
		} else if (val == 0x2601) {
			rv = 0;
		} else {
			err |= 1;
		}
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_config_c, 0, 3, rv);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusFogCoord : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 3)
			err |= 1;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_xf_misc_a, 16, 2, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusFogEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_config_b, 8, 1, val);
				insrt(exp.celsius_xf_misc_b, 31, 1, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusFogColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.celsius_config_c, 8, 1)) {
			warn(4);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_fog_color, 0, 8, extr(val, 16, 8));
				insrt(exp.celsius_fog_color, 8, 8, extr(val, 8, 8));
				insrt(exp.celsius_fog_color, 16, 8, extr(val, 0, 8));
				insrt(exp.celsius_fog_color, 24, 8, extr(val, 24, 8));
			}
		}
		insrt(exp.valid[1], 13, 1, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusTexColorKey : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.celsius_config_c, 8, 1)) {
			warn(4);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				exp.celsius_tex_color_key[idx] = val;
			}
		}
		if (idx == 0)
			insrt(exp.valid[1], 12, 1, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusClipRectMode : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		return val < 2;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			insrt(exp.celsius_config_c, 4, 1, val);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusClipRectHoriz : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			val *= 0x00010001;
		}
		if (rnd() & 1) {
			val &= 0x0fff0fff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		return !(val & 0xf000f000) && extrs(val, 0, 12) <= extrs(val, 16, 12);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			for (int i = idx; i < 8; i++) {
				exp.celsius_clip_rect_horiz[i] = val & 0x0fff0fff;
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusClipRectVert : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			val *= 0x00010001;
		}
		if (rnd() & 1) {
			val &= 0x0fff0fff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		return !(val & 0xf000f000) && extrs(val, 0, 12) <= extrs(val, 16, 12);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			for (int i = idx; i < 8; i++) {
				exp.celsius_clip_rect_vert[i] = val & 0x0fff0fff;
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusAlphaFuncEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_config_a, 12, 1, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusBlendFuncEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_blend, 3, 1, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusCullFaceEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_raster, 28, 1, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusDepthTestEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_config_a, 14, 1, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusDitherEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_config_a, 22, 1, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusLightingEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_xf_misc_b, 29, 1, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusPointParamsEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_config_b, 9, 1, val);
				insrt(exp.celsius_xf_misc_a, 25, 1, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusPointSmoothEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_raster, 9, 1, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusLineSmoothEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_raster, 10, 1, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusPolygonSmoothEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_raster, 11, 1, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusWeightEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_xf_misc_a, 27, 1, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusStencilEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_stencil_func, 0, 1, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusPolygonOffsetPointEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_raster, 6, 1, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusPolygonOffsetLineEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_raster, 7, 1, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusPolygonOffsetFillEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_raster, 8, 1, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusAlphaFuncFunc : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 0x200;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val >= 0x200 && val < 0x208) {
		} else {
			err |= 1;
		}
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_config_a, 8, 4, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusAlphaFuncRef : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xff;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val & ~0xff)
			err |= 2;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_config_a, 0, 8, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusBlendFunc : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= (rnd() & 1 ? 0x8000 : 0x300);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		uint32_t rv = 0;
		switch (val) {
			case 0:
				rv = 0;
				break;
			case 1:
				rv = 1;
				break;
			case 0x300:
				rv = 2;
				break;
			case 0x301:
				rv = 3;
				break;
			case 0x302:
				rv = 4;
				break;
			case 0x303:
				rv = 5;
				break;
			case 0x304:
				rv = 6;
				break;
			case 0x305:
				rv = 7;
				break;
			case 0x306:
				rv = 8;
				break;
			case 0x307:
				rv = 9;
				break;
			case 0x308:
				rv = 0xa;
				break;
			case 0x8001:
				rv = 0xc;
				break;
			case 0x8002:
				rv = 0xd;
				break;
			case 0x8003:
				rv = 0xe;
				break;
			case 0x8004:
				rv = 0xf;
				break;
			default:
				err |= 1;
				break;
		}
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_blend, 4 + which * 4, 4, rv);
			}
		}
	}
public:
	MthdCelsiusBlendFunc(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdCelsiusBlendColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.celsius_config_c, 8, 1)) {
			warn(4);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				exp.celsius_blend_color = val;
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusBlendEquation : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= (rnd() & 1 ? 0x8000 : 0x300);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		uint32_t rv = 0;
		switch (val) {
			case 0x8006:
				rv = 0x2;
				break;
			case 0x8007:
				rv = 0x3;
				break;
			case 0x8008:
				rv = 0x4;
				break;
			case 0x800a:
				rv = 0x0;
				break;
			case 0x800b:
				rv = 0x1;
				break;
			default:
				err |= 1;
				break;
		}
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_blend, 0, 3, rv);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusDepthFunc : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 0x200;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val >= 0x200 && val < 0x208) {
		} else {
			err |= 1;
		}
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_config_a, 16, 4, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusColorMask : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x1010101;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val & ~0x1010101)
			err |= 1;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_config_a, 29, 1, extr(val, 0, 1));
				insrt(exp.celsius_config_a, 28, 1, extr(val, 8, 1));
				insrt(exp.celsius_config_a, 27, 1, extr(val, 16, 1));
				insrt(exp.celsius_config_a, 26, 1, extr(val, 24, 1));
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusDepthWriteEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_config_a, 24, 1, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusStencilVal : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xff;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (which == 0 && val & ~0xff)
			err |= 2;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_stencil_func, 8 + 8 * which, 8, val);
			}
		}
	}
public:
	MthdCelsiusStencilVal(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdCelsiusStencilFunc : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 0x200;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val >= 0x200 && val < 0x208) {
		} else {
			err |= 1;
		}
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_stencil_func, 4, 4, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusStencilOp : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1)
				val &= 0xff0f;
		} else if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= (rnd() & 1 ? 0x1500 : rnd() & 1 ? 0x8500 : 0x1e00);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		uint32_t rv = 0;
		switch (val) {
			case 0:
				rv = 2;
				break;
			case 0x150a:
				rv = 6;
				break;
			case 0x1e00:
				rv = 1;
				break;
			case 0x1e01:
				rv = 3;
				break;
			case 0x1e02:
				rv = 4;
				break;
			case 0x1e03:
				rv = 5;
				break;
			case 0x8507:
				rv = 7;
				break;
			case 0x8508:
				rv = 8;
				break;
			default:
				err |= 1;
		}
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_stencil_op, 4 * which, 4, rv);
			}
		}
	}
public:
	MthdCelsiusStencilOp(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdCelsiusShadeMode : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1)
				val &= 0xf0ff;
		} else if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 0x1d00;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		switch (val) {
			case 0x1d00:
				break;
			case 0x1d01:
				break;
			default:
				err |= 1;
		}
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_config_b, 7, 1, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusLineWidth : public SingleMthdTest {
	void adjust_orig_mthd() override {
		val = sext(val, rnd() & 0x1f);
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (val >= 0x200)
			err |= 2;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				uint32_t rv = val & 0x1ff;
				if (chipset.chipset == 0x10)
					rv &= 0xff;
				insrt(exp.celsius_raster, 12, 9, rv);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusPolygonOffsetFactor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				exp.celsius_polygon_offset_factor = val;
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusPolygonOffsetUnits : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				exp.celsius_polygon_offset_units = val;
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusPolygonMode : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1)
				val &= 0xff0f;
		} else if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 0x1b00;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		uint32_t rv = 0;
		switch (val) {
			case 0x1b00:
				rv = 1;
				break;
			case 0x1b01:
				rv = 2;
				break;
			case 0x1b02:
				rv = 0;
				break;
			default:
				err |= 1;
		}
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_raster, which * 2, 2, rv);
			}
		}
	}
public:
	MthdCelsiusPolygonMode(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdCelsiusDepthRangeNear : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				exp.celsius_depth_range_near = val;
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusDepthRangeFar : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				exp.celsius_depth_range_far = val;
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusCullFace : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1)
				val &= 0xff0f;
		} else if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 0x400;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		uint32_t rv = 0;
		switch (val) {
			case 0x404:
				rv = 1;
				break;
			case 0x405:
				rv = 2;
				break;
			case 0x408:
				rv = 3;
				break;
			default:
				err |= 1;
		}
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_raster, 21, 2, rv);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusFrontFace : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1)
				val &= 0xff0f;
		} else if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 0x900;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		switch (val) {
			case 0x900:
				break;
			case 0x901:
				break;
			default:
				err |= 1;
		}
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_raster, 23, 1, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusNormalizeEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_xf_misc_b, 30, 1, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusSpecularEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_config_b, 5, 1, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusLightEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool is_valid_val() override {
		if (val & ~0xffff)
			return false;
		bool off = false;
		for (int i = 0; i < 8; i++) {
			int mode = extr(val, i * 2, 2);
			if (off && mode != 0)
				return false;
			if (mode == 0)
				off = true;
		}
		return true;
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_xf_misc_a, 0, 16, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusTexGenMode : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1)
				val &= 0xff0f;
		} else if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= (rnd() & 1 ? 0x8550 : rnd() & 1 ? 0x8510 : 0x2400);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		uint32_t rv = 0;
		switch (val) {
			case 0:
				rv = 0;
				break;
			case 0x2400:
				rv = 1;
				break;
			case 0x2401:
				rv = 2;
				break;
			case 0x8511:
				if (which < 3) {
					rv = 4;
					break;
				}
			case 0x8512:
				if (which < 3) {
					rv = 5;
					break;
				}
			case 0x855f:
				if (which < 3) {
					rv = 6;
					break;
				}
			case 0x2402:
				if (which < 2) {
					rv = 3;
					break;
				}
			default:
				err |= 1;
		}
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_xf_misc_b, 3 + idx * 14 + which * 3, which == 3 ? 2 : 3, rv);
			}
		}
	}
public:
	MthdCelsiusTexGenMode(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, uint32_t num, uint32_t stride, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, num, stride), which(which) {}
};

class MthdCelsiusTexMatrixEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_xf_misc_b, 1 + idx * 14, 1, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusTlMode : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (val & ~7)
			err |= 1;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_xf_misc_a, 28, 1, extr(val, 0, 1));
				insrt(exp.celsius_xf_misc_b, 2, 1, extr(val, 1, 1));
				insrt(exp.celsius_xf_misc_b, 16, 1, extr(val, 2, 1));
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusPointSize : public SingleMthdTest {
	void adjust_orig_mthd() override {
		val = sext(val, rnd() & 0x1f);
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (val >= 0x200)
			err |= 2;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				exp.celsius_point_size = val & 0x1ff;
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusUnk3f0 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
		}
		if (rnd() & 1) {
			val |= 1 << (rnd() & 0x1f);
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool is_valid_val() override {
		return val < 4;
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_raster, 26, 2, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusUnk3f4 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
		}
		if (rnd() & 1) {
			val |= 1 << (rnd() & 0x1f);
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool is_valid_val() override {
		return val < 2;
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_config_b, 0, 1, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusOldUnk3f8 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
		}
		if (rnd() & 1) {
			val |= 1 << (rnd() & 0x1f);
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 3)
			err |= 1;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_raster, 30, 2, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusColorLogicOpEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
		}
		if (rnd() & 1) {
			val |= 1 << (rnd() & 0x1f);
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_blend, 16, 1, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusColorLogicOpOp : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 0x1500;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		if (rnd() & 1)
			insrt(exp.notify, 28, 3, 0);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val >= 0x1500 && val < 0x1510) {
		} else {
			err |= 1;
		}
		if (extr(exp.celsius_config_c, 8, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.celsius_blend, 12, 4, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdDmaZcull : public SingleMthdTest {
	bool takes_dma() override { return true; }
	void emulate_mthd() override {
		uint32_t offset_mask = 0x3fffffff;
		uint32_t base = (pobj[2] & ~0xfff) | extr(pobj[0], 20, 12);
		uint32_t limit = pobj[1];
		uint32_t dcls = extr(pobj[0], 0, 12);
		exp.celsius_surf_limit_zcull = (limit & offset_mask) | (dcls != 0x30) << 31;
		exp.celsius_surf_base_zcull = base & offset_mask;
		bool bad = true;
		if (dcls == 0x30 || dcls == 0x3d)
			bad = false;
		if (dcls == 3 && (cls == 0x38 || cls == 0x88))
			bad = false;
		if (extr(exp.debug[3], 23, 1) && bad) {
			nv04_pgraph_blowup(&exp, 0x2);
		}
		bool prot_err = false;
		if (extr(base, 0, 6))
			prot_err = true;
		if (extr(pobj[0], 16, 2))
			prot_err = true;
		if (dcls == 0x30)
			prot_err = false;
		if (prot_err)
			nv04_pgraph_blowup(&exp, 4);
		insrt(exp.ctx_valid, 29, 1, dcls != 0x30 && !(bad && extr(exp.debug[3], 23, 1)));
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdSurfPitchClipid : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xff80;
			val ^= 1 << (rnd() & 0x1f);
		}
		if (!(rnd() & 3)) {
			val = (rnd() & 1) << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return !(val & ~0xff80) && !!val;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1))
			exp.celsius_surf_pitch_clipid = val & 0xffff;
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdSurfOffsetClipid : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffffffc0;
			val ^= 1 << (rnd() & 0x1f);
		}
		if (!(rnd() & 3)) {
			val = (rnd() & 1) << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return !(val & ~0xffffffc0);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1))
			exp.celsius_surf_offset_clipid = val & 0x07ffffff;
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdSurfPitchZcull : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xff80;
			val ^= 1 << (rnd() & 0x1f);
		}
		if (!(rnd() & 3)) {
			val = (rnd() & 1) << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return !(val & ~0xff80) && !!val;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1))
			exp.celsius_surf_pitch_zcull = val & 0xffff;
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdSurfOffsetZcull : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffffffc0;
			val ^= 1 << (rnd() & 0x1f);
		}
		if (!(rnd() & 3)) {
			val = (rnd() & 1) << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return !(val & ~0xffffffc0);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1))
			exp.celsius_surf_offset_zcull = val & 0x3fffffff;
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdInvalidateZcull : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return val < 4 || val == 6;
	}
	void emulate_mthd() override {
		// XXX completely broken
		skip = true;
		if (!extr(exp.nsource, 1, 1)) {
			if (val & 1) {
				exp.zcull_unka00[0] = 0;
				exp.zcull_unka00[1] = 0;
			} else {
				insrt(exp.zcull_unka00[0], 0, 13, 0x1000);
				insrt(exp.zcull_unka00[1], 0, 13, 0x1000);
				insrt(exp.zcull_unka00[0], 16, 13, extr(exp.celsius_clear_hv[0], 16, 16) + 1);
				insrt(exp.zcull_unka00[1], 16, 13, extr(exp.celsius_clear_hv[1], 16, 16) + 1);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdClearZeta : public SingleMthdTest {
	bool is_valid_val() override {
		return !extr(exp.celsius_config_c, 8, 1);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.celsius_clear_zeta = val;
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdClipidEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return val < 2 && !extr(exp.celsius_config_c, 8, 1);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			insrt(exp.celsius_config_d, 6, 1, val);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdClipidId : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return !(val & ~0xf) && !extr(exp.celsius_config_c, 8, 1);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.celsius_clipid_id = val & 0xf;
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdClearHv : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			val *= 0x10001;
		}
		if (rnd() & 1) {
			val &= 0x0fff0fff;
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return !(val & ~0x0fff0fff) && !extr(exp.celsius_config_c, 8, 1) &&
			extr(val, 0, 16) <= extr(val, 16, 16);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.celsius_clear_hv[which] = val & 0x0fff0fff;
		}
	}
public:
	MthdClearHv(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdCelsiusUnkd84 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x8000000f;
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return !(val & ~0x80000003) && !extr(exp.celsius_config_c, 8, 1);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			insrt(exp.celsius_config_d, 1, 2, val);
			insrt(exp.celsius_config_d, 31, 1, extr(val, 31, 1));
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusTexUnk258 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return val < 2;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			insrt(exp.celsius_tex_format[idx], 3, 1, val);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusSurfUnk2b8 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		if (extr(val, 4, 28))
			return false;
		if (extr(val, 0, 2) > 2)
			return false;
		if (extr(val, 2, 2) > 2)
			return false;
		return true;
	}
	void emulate_mthd() override {
		insrt(exp.surf_type, 2, 2, extr(val, 0, 2));
		insrt(exp.surf_type, 6, 2, extr(val, 2, 2));
		insrt(exp.unka10, 29, 1, extr(exp.debug[4], 2, 1) && !!extr(exp.surf_type, 2, 2));
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusSurfUnk2bc : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return val < 2;
	}
	void emulate_mthd() override {
		insrt(exp.surf_type, 31, 1, val);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusUnk3f8 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return val < 2;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			insrt(exp.celsius_config_a, 31, 1, val);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusUnk3fc : public SingleMthdTest {
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.celsius_mthd_unk3fc = val;
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

std::vector<SingleMthdTest *> Celsius::mthds() {
	std::vector<SingleMthdTest *> res = {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdWarning(opt, rnd(), "warning", 2, cls, 0x108),
		new MthdState(opt, rnd(), "state", -1, cls, 0x10c),
		new MthdSync(opt, rnd(), "sync", 1, cls, 0x110),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 3, cls, 0x180),
		new MthdDmaGrobj(opt, rnd(), "dma_tex_a", 4, cls, 0x184, 0, DMA_R | DMA_ALIGN),
		new MthdDmaGrobj(opt, rnd(), "dma_tex_b", 5, cls, 0x188, 1, DMA_R | DMA_ALIGN),
		new MthdDmaVtx(opt, rnd(), "dma_vtx", 6, cls, 0x18c),
		new MthdDmaState(opt, rnd(), "dma_state", 7, cls, 0x190),
		new MthdDmaSurf(opt, rnd(), "dma_surf_color", 8, cls, 0x194, 2, SURF_NV10),
		new MthdDmaSurf(opt, rnd(), "dma_surf_zeta", 9, cls, 0x198, 3, SURF_NV10),
		new MthdClipHv(opt, rnd(), "clip_h", 10, cls, 0x200, 2, 0),
		new MthdClipHv(opt, rnd(), "clip_v", 10, cls, 0x204, 2, 1),
		new MthdSurf3DFormat(opt, rnd(), "surf_format", 11, cls, 0x208, true),
		new MthdSurfPitch2(opt, rnd(), "surf_pitch_2", 12, cls, 0x20c, 2, 3, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "color_offset", 13, cls, 0x210, 2, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "zeta_offset", 14, cls, 0x214, 3, SURF_NV10),
		new MthdCelsiusTexOffset(opt, rnd(), "tex_offset", 15, cls, 0x218, 2),
		new MthdCelsiusTexFormat(opt, rnd(), "tex_format", 16, cls, 0x220, 2),
		new MthdCelsiusTexControl(opt, rnd(), "tex_control", 17, cls, 0x228, 2),
		new MthdCelsiusTexPitch(opt, rnd(), "tex_pitch", 18, cls, 0x230, 2),
		new MthdCelsiusTexUnk238(opt, rnd(), "tex_unk238", 19, cls, 0x238, 2),
		new MthdCelsiusTexRect(opt, rnd(), "tex_rect", 20, cls, 0x240, 2),
		new MthdCelsiusTexFilter(opt, rnd(), "tex_filter", 21, cls, 0x248, 2),
		new MthdCelsiusTexPalette(opt, rnd(), "tex_palette", 22, cls, 0x250, 2),
		new MthdCelsiusRcInAlpha(opt, rnd(), "rc_in_alpha", 23, cls, 0x260, 2),
		new MthdCelsiusRcInColor(opt, rnd(), "rc_in_color", 24, cls, 0x268, 2),
		new MthdCelsiusRcFactor(opt, rnd(), "rc_factor", 25, cls, 0x270, 2),
		new MthdCelsiusRcOutAlpha(opt, rnd(), "rc_out_alpha", 26, cls, 0x278, 2),
		new MthdCelsiusRcOutColor(opt, rnd(), "rc_out_color", 27, cls, 0x280, 2),
		new MthdCelsiusRcFinal0(opt, rnd(), "rc_final_0", 28, cls, 0x288),
		new MthdCelsiusRcFinal1(opt, rnd(), "rc_final_1", 29, cls, 0x28c),
		new MthdCelsiusConfig(opt, rnd(), "config", 30, cls, 0x290),
		new MthdCelsiusLightModel(opt, rnd(), "light_model", 31, cls, 0x294),
		new MthdCelsiusLightMaterial(opt, rnd(), "light_material", -1, cls, 0x298),
		new MthdCelsiusFogMode(opt, rnd(), "fog_mode", -1, cls, 0x29c),
		new MthdCelsiusFogCoord(opt, rnd(), "fog_coord", -1, cls, 0x2a0),
		new MthdCelsiusFogEnable(opt, rnd(), "fog_enable", -1, cls, 0x2a4),
		new MthdCelsiusFogColor(opt, rnd(), "fog_color", -1, cls, 0x2a8),
		new MthdCelsiusTexColorKey(opt, rnd(), "tex_color_key", -1, cls, 0x2ac, 2),
		new MthdCelsiusClipRectMode(opt, rnd(), "clip_rect_mode", -1, cls, 0x2b4),
		new MthdCelsiusClipRectHoriz(opt, rnd(), "clip_rect_horiz", -1, cls, 0x2c0, 8),
		new MthdCelsiusClipRectVert(opt, rnd(), "clip_rect_vert", -1, cls, 0x2e0, 8),
		new MthdCelsiusAlphaFuncEnable(opt, rnd(), "alpha_func_enable", -1, cls, 0x300),
		new MthdCelsiusBlendFuncEnable(opt, rnd(), "blend_func_enable", -1, cls, 0x304),
		new MthdCelsiusCullFaceEnable(opt, rnd(), "cull_face_enable", -1, cls, 0x308),
		new MthdCelsiusDepthTestEnable(opt, rnd(), "depth_test_enable", -1, cls, 0x30c),
		new MthdCelsiusDitherEnable(opt, rnd(), "dither_enable", -1, cls, 0x310),
		new MthdCelsiusLightingEnable(opt, rnd(), "lighting_enable", -1, cls, 0x314),
		new MthdCelsiusPointParamsEnable(opt, rnd(), "point_params_enable", -1, cls, 0x318),
		new MthdCelsiusPointSmoothEnable(opt, rnd(), "point_smooth_enable", -1, cls, 0x31c),
		new MthdCelsiusLineSmoothEnable(opt, rnd(), "line_smooth_enable", -1, cls, 0x320),
		new MthdCelsiusPolygonSmoothEnable(opt, rnd(), "polygon_smooth_enable", -1, cls, 0x324),
		new MthdCelsiusWeightEnable(opt, rnd(), "weight_enable", -1, cls, 0x328),
		new MthdCelsiusStencilEnable(opt, rnd(), "stencil_enable", -1, cls, 0x32c),
		new MthdCelsiusPolygonOffsetPointEnable(opt, rnd(), "polygon_offset_point_enable", -1, cls, 0x330),
		new MthdCelsiusPolygonOffsetLineEnable(opt, rnd(), "polygon_offset_line_enable", -1, cls, 0x334),
		new MthdCelsiusPolygonOffsetFillEnable(opt, rnd(), "polygon_offset_fill_enable", -1, cls, 0x338),
		new MthdCelsiusAlphaFuncFunc(opt, rnd(), "alpha_func_func", -1, cls, 0x33c),
		new MthdCelsiusAlphaFuncRef(opt, rnd(), "alpha_func_ref", -1, cls, 0x340),
		new MthdCelsiusBlendFunc(opt, rnd(), "blend_func_src", -1, cls, 0x344, 0),
		new MthdCelsiusBlendFunc(opt, rnd(), "blend_func_dst", -1, cls, 0x348, 1),
		new MthdCelsiusBlendColor(opt, rnd(), "blend_color", -1, cls, 0x34c),
		new MthdCelsiusBlendEquation(opt, rnd(), "blend_equation", -1, cls, 0x350),
		new MthdCelsiusDepthFunc(opt, rnd(), "depth_func", -1, cls, 0x354),
		new MthdCelsiusColorMask(opt, rnd(), "color_mask", -1, cls, 0x358),
		new MthdCelsiusDepthWriteEnable(opt, rnd(), "depth_write_enable", -1, cls, 0x35c),
		new MthdCelsiusStencilVal(opt, rnd(), "stencil_mask", -1, cls, 0x360, 2),
		new MthdCelsiusStencilFunc(opt, rnd(), "stencil_func", -1, cls, 0x364),
		new MthdCelsiusStencilVal(opt, rnd(), "stencil_func_ref", -1, cls, 0x368, 0),
		new MthdCelsiusStencilVal(opt, rnd(), "stencil_func_mask", -1, cls, 0x36c, 1),
		new MthdCelsiusStencilOp(opt, rnd(), "stencil_op_fail", -1, cls, 0x370, 0),
		new MthdCelsiusStencilOp(opt, rnd(), "stencil_op_zfail", -1, cls, 0x374, 1),
		new MthdCelsiusStencilOp(opt, rnd(), "stencil_op_zpass", -1, cls, 0x378, 2),
		new MthdCelsiusShadeMode(opt, rnd(), "shade_mode", -1, cls, 0x37c),
		new MthdCelsiusLineWidth(opt, rnd(), "line_width", -1, cls, 0x380),
		new MthdCelsiusPolygonOffsetFactor(opt, rnd(), "polygon_offset_factor", -1, cls, 0x384),
		new MthdCelsiusPolygonOffsetUnits(opt, rnd(), "polygon_offset_units", -1, cls, 0x388),
		new MthdCelsiusPolygonMode(opt, rnd(), "polygon_mode_front", -1, cls, 0x38c, 0),
		new MthdCelsiusPolygonMode(opt, rnd(), "polygon_mode_back", -1, cls, 0x390, 1),
		new MthdCelsiusDepthRangeNear(opt, rnd(), "depth_range_near", -1, cls, 0x394),
		new MthdCelsiusDepthRangeFar(opt, rnd(), "depth_range_far", -1, cls, 0x398),
		new MthdCelsiusCullFace(opt, rnd(), "cull_face", -1, cls, 0x39c),
		new MthdCelsiusFrontFace(opt, rnd(), "front_face", -1, cls, 0x3a0),
		new MthdCelsiusNormalizeEnable(opt, rnd(), "normalize_enable", -1, cls, 0x3a4),
		new UntestedMthd(opt, rnd(), "material_factor", -1, cls, 0x3a8, 4), // XXX
		new MthdCelsiusSpecularEnable(opt, rnd(), "specular_enable", -1, cls, 0x3b8),
		new MthdCelsiusLightEnable(opt, rnd(), "light_enable", -1, cls, 0x3bc),
		new MthdCelsiusTexGenMode(opt, rnd(), "tex_gen_mode_s", -1, cls, 0x3c0, 2, 0x10, 0),
		new MthdCelsiusTexGenMode(opt, rnd(), "tex_gen_mode_t", -1, cls, 0x3c4, 2, 0x10, 1),
		new MthdCelsiusTexGenMode(opt, rnd(), "tex_gen_mode_r", -1, cls, 0x3c8, 2, 0x10, 2),
		new MthdCelsiusTexGenMode(opt, rnd(), "tex_gen_mode_q", -1, cls, 0x3cc, 2, 0x10, 3),
		new MthdCelsiusTexMatrixEnable(opt, rnd(), "tex_matrix_enable", -1, cls, 0x3e0, 2),
		new MthdCelsiusTlMode(opt, rnd(), "tl_mode", -1, cls, 0x3e8),
		new MthdCelsiusPointSize(opt, rnd(), "point_size", -1, cls, 0x3ec),
		new MthdCelsiusUnk3f0(opt, rnd(), "unk3f0", -1, cls, 0x3f0),
		new MthdCelsiusUnk3f4(opt, rnd(), "unk3f4", -1, cls, 0x3f4),
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x400, 0x70), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x600, 0x20), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x680, 7), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x6a0, 6), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x6c4, 3), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x6e8, 6), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x700, 0x12), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x800, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x804, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x808, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x80c, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x810, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x814, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x818, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x81c, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x820, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x824, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x828, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x82c, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x830, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x834, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x838, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x83c, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x840, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x844, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x848, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x84c, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x850, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x854, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x858, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x85c, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x860, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x864, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x868, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x86c, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x870, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xc00, 3), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xc10, 11), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xc40, 0x10), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xc80, 7), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xca0, 9), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xcc8, 8), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xcec, 5), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xd00, 0x10), // XXX
		new UntestedMthd(opt, rnd(), "draw_idx16.begin", -1, cls, 0xdfc), // XXX
		new UntestedMthd(opt, rnd(), "draw_idx16.data", -1, cls, 0xe00, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "draw_idx32.begin", -1, cls, 0x10fc), // XXX
		new UntestedMthd(opt, rnd(), "draw_idx32.data", -1, cls, 0x1100, 0x40), // XXX
		new UntestedMthd(opt, rnd(), "draw_arrays.begin", -1, cls, 0x13fc), // XXX
		new UntestedMthd(opt, rnd(), "draw_arrays.data", -1, cls, 0x1400, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "draw_inline.begin", -1, cls, 0x17fc), // XXX
		new UntestedMthd(opt, rnd(), "draw_inline.data", -1, cls, 0x1800, 0x200), // XXX
	};
	if (cls != 0x56) {
		res.insert(res.end(), {
			new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x114), // XXX
			new MthdFlipSet(opt, rnd(), "flip_write", -1, cls, 0x120, 1, 1),
			new MthdFlipSet(opt, rnd(), "flip_read", -1, cls, 0x124, 1, 0),
			new MthdFlipSet(opt, rnd(), "flip_modulo", -1, cls, 0x128, 1, 2),
			new MthdFlipBumpWrite(opt, rnd(), "flip_bump_write", -1, cls, 0x12c, 1),
			new UntestedMthd(opt, rnd(), "flip", -1, cls, 0x130),
			new MthdCelsiusColorLogicOpEnable(opt, rnd(), "color_logic_op_enable", -1, cls, 0xd40),
			new MthdCelsiusColorLogicOpOp(opt, rnd(), "color_logic_op_op", -1, cls, 0xd44),
		});
	}
	if (cls == 0x99) {
		res.insert(res.end(), {
			new MthdDmaClipid(opt, rnd(), "dma_clipid", -1, cls, 0x1ac),
			new MthdDmaZcull(opt, rnd(), "dma_zcull", -1, cls, 0x1b0),
			new MthdSurfPitchClipid(opt, rnd(), "surf_pitch_clipid", -1, cls, 0xd54),
			new MthdSurfOffsetClipid(opt, rnd(), "surf_offset_clipid", -1, cls, 0xd58),
			new MthdSurfPitchZcull(opt, rnd(), "surf_pitch_zcull", -1, cls, 0xd5c),
			new MthdSurfOffsetZcull(opt, rnd(), "surf_offset_zcull", -1, cls, 0xd60),
			new MthdInvalidateZcull(opt, rnd(), "invalidate_zcull", -1, cls, 0xd64),
			new MthdClearZeta(opt, rnd(), "clear_zeta", -1, cls, 0xd68),
			new UntestedMthd(opt, rnd(), "clear_zeta_trigger", -1, cls, 0xd6c), // XXX
			new UntestedMthd(opt, rnd(), "clear_clipid_trigger", -1, cls, 0xd70), // XXX
			new MthdClipidEnable(opt, rnd(), "clipid_enable", -1, cls, 0xd74),
			new MthdClipidId(opt, rnd(), "clipid_id", -1, cls, 0xd78),
			new MthdClearHv(opt, rnd(), "clear_h", -1, cls, 0xd7c, 0),
			new MthdClearHv(opt, rnd(), "clear_v", -1, cls, 0xd80, 1),
			new MthdCelsiusUnkd84(opt, rnd(), "unkd84", -1, cls, 0xd84),
			new MthdCelsiusTexUnk258(opt, rnd(), "tex_unk258", -1, cls, 0x258, 2),
			new MthdCelsiusSurfUnk2b8(opt, rnd(), "surf_unk2b8", -1, cls, 0x2b8),
			new MthdCelsiusSurfUnk2bc(opt, rnd(), "surf_unk2bc", -1, cls, 0x2bc),
			new MthdCelsiusUnk3f8(opt, rnd(), "unk3f8", -1, cls, 0x3f8),
			new MthdCelsiusUnk3fc(opt, rnd(), "unk3fc", -1, cls, 0x3fc),
		});
	} else {
		res.insert(res.end(), {
			new MthdCelsiusOldUnk3f8(opt, rnd(), "old_unk3f8", -1, cls, 0x3f8),
		});
	}
	return res;
}

}
}
