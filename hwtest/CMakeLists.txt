project(ENVYTOOLS C CXX)
cmake_minimum_required(VERSION 2.6)

option(DISABLE_HWTEST "Disable building hwtest" OFF)
if (NOT DISABLE_HWTEST)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(PC_PCIACCESS pciaccess)

	if (PC_PCIACCESS_FOUND)

		include_directories(${PC_PCIACCESS_INCLUDE_DIRS})
		link_directories(${PC_PCIACCESS_LIBRARY_DIRS})

		add_executable(hwtest hwtest.cc
			pgraph.cc pgraph_state.cc pgraph_scan.cc pgraph_state_tests.cc
			pgraph_mthd.cc pgraph_mthd_misc.cc pgraph_mthd_grobj.cc
			pgraph_mthd_xy.cc pgraph_mthd_invalid.cc
			pgraph_class_op.cc pgraph_class_ctx.cc
			pgraph_class_clip.cc pgraph_class_surf.cc
			pgraph_class_solid.cc pgraph_class_ifc.cc
			pgraph_class_ifm.cc pgraph_class_tex.cc
			pgraph_class_m2mf.cc pgraph_class_d3d0.cc
			pgraph_class_sifm.cc pgraph_class_d3d56.cc
			pgraph_class_celsius.cc pgraph_class_emu_d3d56.cc
			pgraph_class_kelvin.cc
			pgraph_rop.cc nv04_pgraph.cc
			pfifo.cc
			vram.cc nv10_tile.cc
			nv50_ptherm.cc nv84_ptherm.cc
			mpeg_crypt.cc vp1.cc vp2_macro.cc pvcomp_isa.cc
			g80_gr.cc g80_sfu.cc g80_int.cc g80_fp.cc g80_fp64.cc g80_atom32.cc g80_atom64.cc
		)
		target_link_libraries(hwtest nva nvhw m)

		install(TARGETS hwtest
			RUNTIME DESTINATION bin
			LIBRARY DESTINATION lib${LIB_SUFFIX}
			ARCHIVE DESTINATION lib${LIB_SUFFIX})

	else(PC_PCIACCESS_FOUND)
		message("Warning: hwtest won't be built because of un-met dependencies (pciaccess)")
	endif(PC_PCIACCESS_FOUND)
endif (NOT DISABLE_HWTEST)
