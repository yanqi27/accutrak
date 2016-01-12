
/////////////////////////////////////////////////////////////////////
// Generated Custom malloc routines
// AccuTrak MalloReplay will import/use these per-module routines
// User are encouraged to play with these routines to achieve best performace/resource
/////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <stdio.h>
#include "CustomMalloc.h"

#ifdef _SMARTHEAP
#include "smrtheap.h"
static MEM_POOL PrivatePools[MAX_MODULES_TO_RECORD] = { NULL };
#endif

void* Malloc_0(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_1(size_t iSize)
{
#if defined(_SMARTHEAP) && defined(_PRIVATE_HEAP)
	//return malloc(iSize);
	if(PrivatePools[1] == NULL)
	{
		if((PrivatePools[1] = MemPoolInit(MEM_POOL_DEFAULT)) == NULL)
		{
			::fprintf(stderr, "Failed to create private heap for module [1]\n");
		}
	}

	// allocate from private heap
	return MemAllocPtr(PrivatePools[1], iSize, 0);
#else
	return malloc(iSize);
#endif
};

void* Malloc_2(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_3(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_4(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_5(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_6(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_7(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_8(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_9(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_10(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_11(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_12(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_13(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_14(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_15(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_16(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_17(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_18(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_19(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_20(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_21(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_22(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_23(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_24(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_25(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_26(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_27(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_28(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_29(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_30(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_31(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_32(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_33(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_34(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_35(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_36(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_37(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_38(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_39(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_40(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_41(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_42(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_43(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_44(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_45(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_46(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_47(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_48(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_49(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_50(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_51(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_52(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_53(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_54(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_55(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_56(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_57(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_58(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_59(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_60(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_61(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_62(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_63(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_64(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_65(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_66(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_67(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_68(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_69(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_70(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_71(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_72(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_73(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_74(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_75(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_76(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_77(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_78(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_79(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_80(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_81(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_82(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_83(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_84(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_85(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_86(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_87(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_88(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_89(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_90(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_91(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_92(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_93(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_94(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_95(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_96(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_97(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_98(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_99(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_100(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_101(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_102(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_103(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_104(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_105(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_106(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_107(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_108(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_109(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_110(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_111(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_112(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_113(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_114(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_115(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_116(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_117(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_118(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_119(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_120(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_121(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_122(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_123(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_124(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_125(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_126(size_t iSize)
{
#if defined(_SMARTHEAP) && defined(_PRIVATE_HEAP)
	//return malloc(iSize);
	if(PrivatePools[126] == NULL)
	{
		if((PrivatePools[126] = MemPoolInit(MEM_POOL_DEFAULT)) == NULL)
		{
			::fprintf(stderr, "Failed to create private heap for module [1]\n");
		}
	}

	// allocate from private heap
	return MemAllocPtr(PrivatePools[126], iSize, 0);
#else
	return malloc(iSize);
#endif
};

void* Malloc_127(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_128(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_129(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_130(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_131(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_132(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_133(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_134(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_135(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_136(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_137(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_138(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_139(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_140(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_141(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_142(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_143(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_144(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_145(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_146(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_147(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_148(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_149(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_150(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_151(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_152(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_153(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_154(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_155(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_156(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_157(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_158(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_159(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_160(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_161(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_162(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_163(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_164(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_165(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_166(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_167(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_168(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_169(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_170(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_171(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_172(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_173(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_174(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_175(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_176(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_177(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_178(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_179(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_180(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_181(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_182(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_183(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_184(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_185(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_186(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_187(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_188(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_189(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_190(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_191(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_192(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_193(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_194(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_195(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_196(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_197(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_198(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_199(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_200(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_201(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_202(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_203(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_204(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_205(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_206(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_207(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_208(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_209(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_210(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_211(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_212(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_213(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_214(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_215(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_216(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_217(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_218(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_219(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_220(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_221(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_222(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_223(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_224(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_225(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_226(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_227(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_228(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_229(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_230(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_231(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_232(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_233(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_234(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_235(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_236(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_237(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_238(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_239(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_240(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_241(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_242(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_243(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_244(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_245(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_246(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_247(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_248(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_249(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_250(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_251(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_252(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_253(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_254(size_t iSize)
{
	return malloc(iSize);
};

void* Malloc_255(size_t iSize)
{
	return malloc(iSize);
};

void Free_0(void* ipBlock)
{
	free(ipBlock);
};

void Free_1(void* ipBlock)
{
	free(ipBlock);
};

void Free_2(void* ipBlock)
{
	free(ipBlock);
};

void Free_3(void* ipBlock)
{
	free(ipBlock);
};

void Free_4(void* ipBlock)
{
	free(ipBlock);
};

void Free_5(void* ipBlock)
{
	free(ipBlock);
};

void Free_6(void* ipBlock)
{
	free(ipBlock);
};

void Free_7(void* ipBlock)
{
	free(ipBlock);
};

void Free_8(void* ipBlock)
{
	free(ipBlock);
};

void Free_9(void* ipBlock)
{
	free(ipBlock);
};

void Free_10(void* ipBlock)
{
	free(ipBlock);
};

void Free_11(void* ipBlock)
{
	free(ipBlock);
};

void Free_12(void* ipBlock)
{
	free(ipBlock);
};

void Free_13(void* ipBlock)
{
	free(ipBlock);
};

void Free_14(void* ipBlock)
{
	free(ipBlock);
};

void Free_15(void* ipBlock)
{
	free(ipBlock);
};

void Free_16(void* ipBlock)
{
	free(ipBlock);
};

void Free_17(void* ipBlock)
{
	free(ipBlock);
};

void Free_18(void* ipBlock)
{
	free(ipBlock);
};

void Free_19(void* ipBlock)
{
	free(ipBlock);
};

void Free_20(void* ipBlock)
{
	free(ipBlock);
};

void Free_21(void* ipBlock)
{
	free(ipBlock);
};

void Free_22(void* ipBlock)
{
	free(ipBlock);
};

void Free_23(void* ipBlock)
{
	free(ipBlock);
};

void Free_24(void* ipBlock)
{
	free(ipBlock);
};

void Free_25(void* ipBlock)
{
	free(ipBlock);
};

void Free_26(void* ipBlock)
{
	free(ipBlock);
};

void Free_27(void* ipBlock)
{
	free(ipBlock);
};

void Free_28(void* ipBlock)
{
	free(ipBlock);
};

void Free_29(void* ipBlock)
{
	free(ipBlock);
};

void Free_30(void* ipBlock)
{
	free(ipBlock);
};

void Free_31(void* ipBlock)
{
	free(ipBlock);
};

void Free_32(void* ipBlock)
{
	free(ipBlock);
};

void Free_33(void* ipBlock)
{
	free(ipBlock);
};

void Free_34(void* ipBlock)
{
	free(ipBlock);
};

void Free_35(void* ipBlock)
{
	free(ipBlock);
};

void Free_36(void* ipBlock)
{
	free(ipBlock);
};

void Free_37(void* ipBlock)
{
	free(ipBlock);
};

void Free_38(void* ipBlock)
{
	free(ipBlock);
};

void Free_39(void* ipBlock)
{
	free(ipBlock);
};

void Free_40(void* ipBlock)
{
	free(ipBlock);
};

void Free_41(void* ipBlock)
{
	free(ipBlock);
};

void Free_42(void* ipBlock)
{
	free(ipBlock);
};

void Free_43(void* ipBlock)
{
	free(ipBlock);
};

void Free_44(void* ipBlock)
{
	free(ipBlock);
};

void Free_45(void* ipBlock)
{
	free(ipBlock);
};

void Free_46(void* ipBlock)
{
	free(ipBlock);
};

void Free_47(void* ipBlock)
{
	free(ipBlock);
};

void Free_48(void* ipBlock)
{
	free(ipBlock);
};

void Free_49(void* ipBlock)
{
	free(ipBlock);
};

void Free_50(void* ipBlock)
{
	free(ipBlock);
};

void Free_51(void* ipBlock)
{
	free(ipBlock);
};

void Free_52(void* ipBlock)
{
	free(ipBlock);
};

void Free_53(void* ipBlock)
{
	free(ipBlock);
};

void Free_54(void* ipBlock)
{
	free(ipBlock);
};

void Free_55(void* ipBlock)
{
	free(ipBlock);
};

void Free_56(void* ipBlock)
{
	free(ipBlock);
};

void Free_57(void* ipBlock)
{
	free(ipBlock);
};

void Free_58(void* ipBlock)
{
	free(ipBlock);
};

void Free_59(void* ipBlock)
{
	free(ipBlock);
};

void Free_60(void* ipBlock)
{
	free(ipBlock);
};

void Free_61(void* ipBlock)
{
	free(ipBlock);
};

void Free_62(void* ipBlock)
{
	free(ipBlock);
};

void Free_63(void* ipBlock)
{
	free(ipBlock);
};

void Free_64(void* ipBlock)
{
	free(ipBlock);
};

void Free_65(void* ipBlock)
{
	free(ipBlock);
};

void Free_66(void* ipBlock)
{
	free(ipBlock);
};

void Free_67(void* ipBlock)
{
	free(ipBlock);
};

void Free_68(void* ipBlock)
{
	free(ipBlock);
};

void Free_69(void* ipBlock)
{
	free(ipBlock);
};

void Free_70(void* ipBlock)
{
	free(ipBlock);
};

void Free_71(void* ipBlock)
{
	free(ipBlock);
};

void Free_72(void* ipBlock)
{
	free(ipBlock);
};

void Free_73(void* ipBlock)
{
	free(ipBlock);
};

void Free_74(void* ipBlock)
{
	free(ipBlock);
};

void Free_75(void* ipBlock)
{
	free(ipBlock);
};

void Free_76(void* ipBlock)
{
	free(ipBlock);
};

void Free_77(void* ipBlock)
{
	free(ipBlock);
};

void Free_78(void* ipBlock)
{
	free(ipBlock);
};

void Free_79(void* ipBlock)
{
	free(ipBlock);
};

void Free_80(void* ipBlock)
{
	free(ipBlock);
};

void Free_81(void* ipBlock)
{
	free(ipBlock);
};

void Free_82(void* ipBlock)
{
	free(ipBlock);
};

void Free_83(void* ipBlock)
{
	free(ipBlock);
};

void Free_84(void* ipBlock)
{
	free(ipBlock);
};

void Free_85(void* ipBlock)
{
	free(ipBlock);
};

void Free_86(void* ipBlock)
{
	free(ipBlock);
};

void Free_87(void* ipBlock)
{
	free(ipBlock);
};

void Free_88(void* ipBlock)
{
	free(ipBlock);
};

void Free_89(void* ipBlock)
{
	free(ipBlock);
};

void Free_90(void* ipBlock)
{
	free(ipBlock);
};

void Free_91(void* ipBlock)
{
	free(ipBlock);
};

void Free_92(void* ipBlock)
{
	free(ipBlock);
};

void Free_93(void* ipBlock)
{
	free(ipBlock);
};

void Free_94(void* ipBlock)
{
	free(ipBlock);
};

void Free_95(void* ipBlock)
{
	free(ipBlock);
};

void Free_96(void* ipBlock)
{
	free(ipBlock);
};

void Free_97(void* ipBlock)
{
	free(ipBlock);
};

void Free_98(void* ipBlock)
{
	free(ipBlock);
};

void Free_99(void* ipBlock)
{
	free(ipBlock);
};

void Free_100(void* ipBlock)
{
	free(ipBlock);
};

void Free_101(void* ipBlock)
{
	free(ipBlock);
};

void Free_102(void* ipBlock)
{
	free(ipBlock);
};

void Free_103(void* ipBlock)
{
	free(ipBlock);
};

void Free_104(void* ipBlock)
{
	free(ipBlock);
};

void Free_105(void* ipBlock)
{
	free(ipBlock);
};

void Free_106(void* ipBlock)
{
	free(ipBlock);
};

void Free_107(void* ipBlock)
{
	free(ipBlock);
};

void Free_108(void* ipBlock)
{
	free(ipBlock);
};

void Free_109(void* ipBlock)
{
	free(ipBlock);
};

void Free_110(void* ipBlock)
{
	free(ipBlock);
};

void Free_111(void* ipBlock)
{
	free(ipBlock);
};

void Free_112(void* ipBlock)
{
	free(ipBlock);
};

void Free_113(void* ipBlock)
{
	free(ipBlock);
};

void Free_114(void* ipBlock)
{
	free(ipBlock);
};

void Free_115(void* ipBlock)
{
	free(ipBlock);
};

void Free_116(void* ipBlock)
{
	free(ipBlock);
};

void Free_117(void* ipBlock)
{
	free(ipBlock);
};

void Free_118(void* ipBlock)
{
	free(ipBlock);
};

void Free_119(void* ipBlock)
{
	free(ipBlock);
};

void Free_120(void* ipBlock)
{
	free(ipBlock);
};

void Free_121(void* ipBlock)
{
	free(ipBlock);
};

void Free_122(void* ipBlock)
{
	free(ipBlock);
};

void Free_123(void* ipBlock)
{
	free(ipBlock);
};

void Free_124(void* ipBlock)
{
	free(ipBlock);
};

void Free_125(void* ipBlock)
{
	free(ipBlock);
};

void Free_126(void* ipBlock)
{
	free(ipBlock);
};

void Free_127(void* ipBlock)
{
	free(ipBlock);
};

void Free_128(void* ipBlock)
{
	free(ipBlock);
};

void Free_129(void* ipBlock)
{
	free(ipBlock);
};

void Free_130(void* ipBlock)
{
	free(ipBlock);
};

void Free_131(void* ipBlock)
{
	free(ipBlock);
};

void Free_132(void* ipBlock)
{
	free(ipBlock);
};

void Free_133(void* ipBlock)
{
	free(ipBlock);
};

void Free_134(void* ipBlock)
{
	free(ipBlock);
};

void Free_135(void* ipBlock)
{
	free(ipBlock);
};

void Free_136(void* ipBlock)
{
	free(ipBlock);
};

void Free_137(void* ipBlock)
{
	free(ipBlock);
};

void Free_138(void* ipBlock)
{
	free(ipBlock);
};

void Free_139(void* ipBlock)
{
	free(ipBlock);
};

void Free_140(void* ipBlock)
{
	free(ipBlock);
};

void Free_141(void* ipBlock)
{
	free(ipBlock);
};

void Free_142(void* ipBlock)
{
	free(ipBlock);
};

void Free_143(void* ipBlock)
{
	free(ipBlock);
};

void Free_144(void* ipBlock)
{
	free(ipBlock);
};

void Free_145(void* ipBlock)
{
	free(ipBlock);
};

void Free_146(void* ipBlock)
{
	free(ipBlock);
};

void Free_147(void* ipBlock)
{
	free(ipBlock);
};

void Free_148(void* ipBlock)
{
	free(ipBlock);
};

void Free_149(void* ipBlock)
{
	free(ipBlock);
};

void Free_150(void* ipBlock)
{
	free(ipBlock);
};

void Free_151(void* ipBlock)
{
	free(ipBlock);
};

void Free_152(void* ipBlock)
{
	free(ipBlock);
};

void Free_153(void* ipBlock)
{
	free(ipBlock);
};

void Free_154(void* ipBlock)
{
	free(ipBlock);
};

void Free_155(void* ipBlock)
{
	free(ipBlock);
};

void Free_156(void* ipBlock)
{
	free(ipBlock);
};

void Free_157(void* ipBlock)
{
	free(ipBlock);
};

void Free_158(void* ipBlock)
{
	free(ipBlock);
};

void Free_159(void* ipBlock)
{
	free(ipBlock);
};

void Free_160(void* ipBlock)
{
	free(ipBlock);
};

void Free_161(void* ipBlock)
{
	free(ipBlock);
};

void Free_162(void* ipBlock)
{
	free(ipBlock);
};

void Free_163(void* ipBlock)
{
	free(ipBlock);
};

void Free_164(void* ipBlock)
{
	free(ipBlock);
};

void Free_165(void* ipBlock)
{
	free(ipBlock);
};

void Free_166(void* ipBlock)
{
	free(ipBlock);
};

void Free_167(void* ipBlock)
{
	free(ipBlock);
};

void Free_168(void* ipBlock)
{
	free(ipBlock);
};

void Free_169(void* ipBlock)
{
	free(ipBlock);
};

void Free_170(void* ipBlock)
{
	free(ipBlock);
};

void Free_171(void* ipBlock)
{
	free(ipBlock);
};

void Free_172(void* ipBlock)
{
	free(ipBlock);
};

void Free_173(void* ipBlock)
{
	free(ipBlock);
};

void Free_174(void* ipBlock)
{
	free(ipBlock);
};

void Free_175(void* ipBlock)
{
	free(ipBlock);
};

void Free_176(void* ipBlock)
{
	free(ipBlock);
};

void Free_177(void* ipBlock)
{
	free(ipBlock);
};

void Free_178(void* ipBlock)
{
	free(ipBlock);
};

void Free_179(void* ipBlock)
{
	free(ipBlock);
};

void Free_180(void* ipBlock)
{
	free(ipBlock);
};

void Free_181(void* ipBlock)
{
	free(ipBlock);
};

void Free_182(void* ipBlock)
{
	free(ipBlock);
};

void Free_183(void* ipBlock)
{
	free(ipBlock);
};

void Free_184(void* ipBlock)
{
	free(ipBlock);
};

void Free_185(void* ipBlock)
{
	free(ipBlock);
};

void Free_186(void* ipBlock)
{
	free(ipBlock);
};

void Free_187(void* ipBlock)
{
	free(ipBlock);
};

void Free_188(void* ipBlock)
{
	free(ipBlock);
};

void Free_189(void* ipBlock)
{
	free(ipBlock);
};

void Free_190(void* ipBlock)
{
	free(ipBlock);
};

void Free_191(void* ipBlock)
{
	free(ipBlock);
};

void Free_192(void* ipBlock)
{
	free(ipBlock);
};

void Free_193(void* ipBlock)
{
	free(ipBlock);
};

void Free_194(void* ipBlock)
{
	free(ipBlock);
};

void Free_195(void* ipBlock)
{
	free(ipBlock);
};

void Free_196(void* ipBlock)
{
	free(ipBlock);
};

void Free_197(void* ipBlock)
{
	free(ipBlock);
};

void Free_198(void* ipBlock)
{
	free(ipBlock);
};

void Free_199(void* ipBlock)
{
	free(ipBlock);
};

void Free_200(void* ipBlock)
{
	free(ipBlock);
};

void Free_201(void* ipBlock)
{
	free(ipBlock);
};

void Free_202(void* ipBlock)
{
	free(ipBlock);
};

void Free_203(void* ipBlock)
{
	free(ipBlock);
};

void Free_204(void* ipBlock)
{
	free(ipBlock);
};

void Free_205(void* ipBlock)
{
	free(ipBlock);
};

void Free_206(void* ipBlock)
{
	free(ipBlock);
};

void Free_207(void* ipBlock)
{
	free(ipBlock);
};

void Free_208(void* ipBlock)
{
	free(ipBlock);
};

void Free_209(void* ipBlock)
{
	free(ipBlock);
};

void Free_210(void* ipBlock)
{
	free(ipBlock);
};

void Free_211(void* ipBlock)
{
	free(ipBlock);
};

void Free_212(void* ipBlock)
{
	free(ipBlock);
};

void Free_213(void* ipBlock)
{
	free(ipBlock);
};

void Free_214(void* ipBlock)
{
	free(ipBlock);
};

void Free_215(void* ipBlock)
{
	free(ipBlock);
};

void Free_216(void* ipBlock)
{
	free(ipBlock);
};

void Free_217(void* ipBlock)
{
	free(ipBlock);
};

void Free_218(void* ipBlock)
{
	free(ipBlock);
};

void Free_219(void* ipBlock)
{
	free(ipBlock);
};

void Free_220(void* ipBlock)
{
	free(ipBlock);
};

void Free_221(void* ipBlock)
{
	free(ipBlock);
};

void Free_222(void* ipBlock)
{
	free(ipBlock);
};

void Free_223(void* ipBlock)
{
	free(ipBlock);
};

void Free_224(void* ipBlock)
{
	free(ipBlock);
};

void Free_225(void* ipBlock)
{
	free(ipBlock);
};

void Free_226(void* ipBlock)
{
	free(ipBlock);
};

void Free_227(void* ipBlock)
{
	free(ipBlock);
};

void Free_228(void* ipBlock)
{
	free(ipBlock);
};

void Free_229(void* ipBlock)
{
	free(ipBlock);
};

void Free_230(void* ipBlock)
{
	free(ipBlock);
};

void Free_231(void* ipBlock)
{
	free(ipBlock);
};

void Free_232(void* ipBlock)
{
	free(ipBlock);
};

void Free_233(void* ipBlock)
{
	free(ipBlock);
};

void Free_234(void* ipBlock)
{
	free(ipBlock);
};

void Free_235(void* ipBlock)
{
	free(ipBlock);
};

void Free_236(void* ipBlock)
{
	free(ipBlock);
};

void Free_237(void* ipBlock)
{
	free(ipBlock);
};

void Free_238(void* ipBlock)
{
	free(ipBlock);
};

void Free_239(void* ipBlock)
{
	free(ipBlock);
};

void Free_240(void* ipBlock)
{
	free(ipBlock);
};

void Free_241(void* ipBlock)
{
	free(ipBlock);
};

void Free_242(void* ipBlock)
{
	free(ipBlock);
};

void Free_243(void* ipBlock)
{
	free(ipBlock);
};

void Free_244(void* ipBlock)
{
	free(ipBlock);
};

void Free_245(void* ipBlock)
{
	free(ipBlock);
};

void Free_246(void* ipBlock)
{
	free(ipBlock);
};

void Free_247(void* ipBlock)
{
	free(ipBlock);
};

void Free_248(void* ipBlock)
{
	free(ipBlock);
};

void Free_249(void* ipBlock)
{
	free(ipBlock);
};

void Free_250(void* ipBlock)
{
	free(ipBlock);
};

void Free_251(void* ipBlock)
{
	free(ipBlock);
};

void Free_252(void* ipBlock)
{
	free(ipBlock);
};

void Free_253(void* ipBlock)
{
	free(ipBlock);
};

void Free_254(void* ipBlock)
{
	free(ipBlock);
};

void Free_255(void* ipBlock)
{
	free(ipBlock);
};

void* Realloc_0(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_1(void* ipBlock, size_t iNewSize)
{
#if defined(_SMARTHEAP) && defined(_PRIVATE_HEAP)
	// allocate from the heap where the old block was allocated from
	return MemReAllocPtr(ipBlock, iNewSize, 0);
#else
	return realloc(ipBlock, iNewSize);
#endif
};

void* Realloc_2(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_3(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_4(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_5(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_6(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_7(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_8(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_9(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_10(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_11(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_12(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_13(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_14(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_15(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_16(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_17(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_18(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_19(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_20(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_21(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_22(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_23(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_24(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_25(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_26(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_27(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_28(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_29(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_30(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_31(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_32(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_33(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_34(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_35(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_36(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_37(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_38(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_39(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_40(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_41(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_42(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_43(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_44(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_45(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_46(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_47(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_48(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_49(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_50(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_51(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_52(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_53(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_54(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_55(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_56(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_57(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_58(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_59(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_60(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_61(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_62(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_63(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_64(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_65(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_66(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_67(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_68(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_69(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_70(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_71(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_72(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_73(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_74(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_75(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_76(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_77(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_78(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_79(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_80(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_81(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_82(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_83(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_84(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_85(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_86(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_87(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_88(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_89(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_90(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_91(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_92(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_93(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_94(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_95(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_96(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_97(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_98(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_99(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_100(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_101(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_102(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_103(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_104(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_105(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_106(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_107(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_108(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_109(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_110(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_111(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_112(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_113(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_114(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_115(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_116(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_117(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_118(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_119(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_120(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_121(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_122(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_123(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_124(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_125(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_126(void* ipBlock, size_t iNewSize)
{
#if defined(_SMARTHEAP) && defined(_PRIVATE_HEAP)
	// allocate from the heap where the old block was allocated from
	return MemReAllocPtr(ipBlock, iNewSize, 0);
#else
	return realloc(ipBlock, iNewSize);
#endif
};

void* Realloc_127(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_128(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_129(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_130(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_131(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_132(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_133(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_134(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_135(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_136(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_137(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_138(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_139(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_140(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_141(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_142(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_143(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_144(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_145(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_146(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_147(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_148(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_149(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_150(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_151(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_152(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_153(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_154(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_155(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_156(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_157(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_158(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_159(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_160(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_161(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_162(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_163(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_164(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_165(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_166(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_167(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_168(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_169(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_170(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_171(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_172(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_173(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_174(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_175(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_176(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_177(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_178(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_179(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_180(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_181(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_182(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_183(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_184(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_185(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_186(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_187(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_188(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_189(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_190(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_191(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_192(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_193(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_194(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_195(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_196(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_197(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_198(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_199(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_200(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_201(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_202(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_203(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_204(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_205(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_206(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_207(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_208(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_209(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_210(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_211(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_212(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_213(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_214(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_215(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_216(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_217(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_218(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_219(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_220(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_221(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_222(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_223(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_224(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_225(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_226(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_227(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_228(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_229(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_230(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_231(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_232(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_233(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_234(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_235(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_236(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_237(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_238(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_239(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_240(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_241(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_242(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_243(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_244(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_245(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_246(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_247(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_248(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_249(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_250(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_251(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_252(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_253(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_254(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};

void* Realloc_255(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
};



/////////////////////////////////////////////////////////////////////
// Array of routines index by module number
/////////////////////////////////////////////////////////////////////

void* (* CustomMallocs[MAX_MODULES_TO_RECORD]) (size_t) =
{
	Malloc_0,
	Malloc_1,
	Malloc_2,
	Malloc_3,
	Malloc_4,
	Malloc_5,
	Malloc_6,
	Malloc_7,
	Malloc_8,
	Malloc_9,
	Malloc_10,
	Malloc_11,
	Malloc_12,
	Malloc_13,
	Malloc_14,
	Malloc_15,
	Malloc_16,
	Malloc_17,
	Malloc_18,
	Malloc_19,
	Malloc_20,
	Malloc_21,
	Malloc_22,
	Malloc_23,
	Malloc_24,
	Malloc_25,
	Malloc_26,
	Malloc_27,
	Malloc_28,
	Malloc_29,
	Malloc_30,
	Malloc_31,
	Malloc_32,
	Malloc_33,
	Malloc_34,
	Malloc_35,
	Malloc_36,
	Malloc_37,
	Malloc_38,
	Malloc_39,
	Malloc_40,
	Malloc_41,
	Malloc_42,
	Malloc_43,
	Malloc_44,
	Malloc_45,
	Malloc_46,
	Malloc_47,
	Malloc_48,
	Malloc_49,
	Malloc_50,
	Malloc_51,
	Malloc_52,
	Malloc_53,
	Malloc_54,
	Malloc_55,
	Malloc_56,
	Malloc_57,
	Malloc_58,
	Malloc_59,
	Malloc_60,
	Malloc_61,
	Malloc_62,
	Malloc_63,
	Malloc_64,
	Malloc_65,
	Malloc_66,
	Malloc_67,
	Malloc_68,
	Malloc_69,
	Malloc_70,
	Malloc_71,
	Malloc_72,
	Malloc_73,
	Malloc_74,
	Malloc_75,
	Malloc_76,
	Malloc_77,
	Malloc_78,
	Malloc_79,
	Malloc_80,
	Malloc_81,
	Malloc_82,
	Malloc_83,
	Malloc_84,
	Malloc_85,
	Malloc_86,
	Malloc_87,
	Malloc_88,
	Malloc_89,
	Malloc_90,
	Malloc_91,
	Malloc_92,
	Malloc_93,
	Malloc_94,
	Malloc_95,
	Malloc_96,
	Malloc_97,
	Malloc_98,
	Malloc_99,
	Malloc_100,
	Malloc_101,
	Malloc_102,
	Malloc_103,
	Malloc_104,
	Malloc_105,
	Malloc_106,
	Malloc_107,
	Malloc_108,
	Malloc_109,
	Malloc_110,
	Malloc_111,
	Malloc_112,
	Malloc_113,
	Malloc_114,
	Malloc_115,
	Malloc_116,
	Malloc_117,
	Malloc_118,
	Malloc_119,
	Malloc_120,
	Malloc_121,
	Malloc_122,
	Malloc_123,
	Malloc_124,
	Malloc_125,
	Malloc_126,
	Malloc_127,
	Malloc_128,
	Malloc_129,
	Malloc_130,
	Malloc_131,
	Malloc_132,
	Malloc_133,
	Malloc_134,
	Malloc_135,
	Malloc_136,
	Malloc_137,
	Malloc_138,
	Malloc_139,
	Malloc_140,
	Malloc_141,
	Malloc_142,
	Malloc_143,
	Malloc_144,
	Malloc_145,
	Malloc_146,
	Malloc_147,
	Malloc_148,
	Malloc_149,
	Malloc_150,
	Malloc_151,
	Malloc_152,
	Malloc_153,
	Malloc_154,
	Malloc_155,
	Malloc_156,
	Malloc_157,
	Malloc_158,
	Malloc_159,
	Malloc_160,
	Malloc_161,
	Malloc_162,
	Malloc_163,
	Malloc_164,
	Malloc_165,
	Malloc_166,
	Malloc_167,
	Malloc_168,
	Malloc_169,
	Malloc_170,
	Malloc_171,
	Malloc_172,
	Malloc_173,
	Malloc_174,
	Malloc_175,
	Malloc_176,
	Malloc_177,
	Malloc_178,
	Malloc_179,
	Malloc_180,
	Malloc_181,
	Malloc_182,
	Malloc_183,
	Malloc_184,
	Malloc_185,
	Malloc_186,
	Malloc_187,
	Malloc_188,
	Malloc_189,
	Malloc_190,
	Malloc_191,
	Malloc_192,
	Malloc_193,
	Malloc_194,
	Malloc_195,
	Malloc_196,
	Malloc_197,
	Malloc_198,
	Malloc_199,
	Malloc_200,
	Malloc_201,
	Malloc_202,
	Malloc_203,
	Malloc_204,
	Malloc_205,
	Malloc_206,
	Malloc_207,
	Malloc_208,
	Malloc_209,
	Malloc_210,
	Malloc_211,
	Malloc_212,
	Malloc_213,
	Malloc_214,
	Malloc_215,
	Malloc_216,
	Malloc_217,
	Malloc_218,
	Malloc_219,
	Malloc_220,
	Malloc_221,
	Malloc_222,
	Malloc_223,
	Malloc_224,
	Malloc_225,
	Malloc_226,
	Malloc_227,
	Malloc_228,
	Malloc_229,
	Malloc_230,
	Malloc_231,
	Malloc_232,
	Malloc_233,
	Malloc_234,
	Malloc_235,
	Malloc_236,
	Malloc_237,
	Malloc_238,
	Malloc_239,
	Malloc_240,
	Malloc_241,
	Malloc_242,
	Malloc_243,
	Malloc_244,
	Malloc_245,
	Malloc_246,
	Malloc_247,
	Malloc_248,
	Malloc_249,
	Malloc_250,
	Malloc_251,
	Malloc_252,
	Malloc_253,
	Malloc_254,
	Malloc_255
};

void  (* CustomFrees[MAX_MODULES_TO_RECORD]) (void*) = 
{
	Free_0,
	Free_1,
	Free_2,
	Free_3,
	Free_4,
	Free_5,
	Free_6,
	Free_7,
	Free_8,
	Free_9,
	Free_10,
	Free_11,
	Free_12,
	Free_13,
	Free_14,
	Free_15,
	Free_16,
	Free_17,
	Free_18,
	Free_19,
	Free_20,
	Free_21,
	Free_22,
	Free_23,
	Free_24,
	Free_25,
	Free_26,
	Free_27,
	Free_28,
	Free_29,
	Free_30,
	Free_31,
	Free_32,
	Free_33,
	Free_34,
	Free_35,
	Free_36,
	Free_37,
	Free_38,
	Free_39,
	Free_40,
	Free_41,
	Free_42,
	Free_43,
	Free_44,
	Free_45,
	Free_46,
	Free_47,
	Free_48,
	Free_49,
	Free_50,
	Free_51,
	Free_52,
	Free_53,
	Free_54,
	Free_55,
	Free_56,
	Free_57,
	Free_58,
	Free_59,
	Free_60,
	Free_61,
	Free_62,
	Free_63,
	Free_64,
	Free_65,
	Free_66,
	Free_67,
	Free_68,
	Free_69,
	Free_70,
	Free_71,
	Free_72,
	Free_73,
	Free_74,
	Free_75,
	Free_76,
	Free_77,
	Free_78,
	Free_79,
	Free_80,
	Free_81,
	Free_82,
	Free_83,
	Free_84,
	Free_85,
	Free_86,
	Free_87,
	Free_88,
	Free_89,
	Free_90,
	Free_91,
	Free_92,
	Free_93,
	Free_94,
	Free_95,
	Free_96,
	Free_97,
	Free_98,
	Free_99,
	Free_100,
	Free_101,
	Free_102,
	Free_103,
	Free_104,
	Free_105,
	Free_106,
	Free_107,
	Free_108,
	Free_109,
	Free_110,
	Free_111,
	Free_112,
	Free_113,
	Free_114,
	Free_115,
	Free_116,
	Free_117,
	Free_118,
	Free_119,
	Free_120,
	Free_121,
	Free_122,
	Free_123,
	Free_124,
	Free_125,
	Free_126,
	Free_127,
	Free_128,
	Free_129,
	Free_130,
	Free_131,
	Free_132,
	Free_133,
	Free_134,
	Free_135,
	Free_136,
	Free_137,
	Free_138,
	Free_139,
	Free_140,
	Free_141,
	Free_142,
	Free_143,
	Free_144,
	Free_145,
	Free_146,
	Free_147,
	Free_148,
	Free_149,
	Free_150,
	Free_151,
	Free_152,
	Free_153,
	Free_154,
	Free_155,
	Free_156,
	Free_157,
	Free_158,
	Free_159,
	Free_160,
	Free_161,
	Free_162,
	Free_163,
	Free_164,
	Free_165,
	Free_166,
	Free_167,
	Free_168,
	Free_169,
	Free_170,
	Free_171,
	Free_172,
	Free_173,
	Free_174,
	Free_175,
	Free_176,
	Free_177,
	Free_178,
	Free_179,
	Free_180,
	Free_181,
	Free_182,
	Free_183,
	Free_184,
	Free_185,
	Free_186,
	Free_187,
	Free_188,
	Free_189,
	Free_190,
	Free_191,
	Free_192,
	Free_193,
	Free_194,
	Free_195,
	Free_196,
	Free_197,
	Free_198,
	Free_199,
	Free_200,
	Free_201,
	Free_202,
	Free_203,
	Free_204,
	Free_205,
	Free_206,
	Free_207,
	Free_208,
	Free_209,
	Free_210,
	Free_211,
	Free_212,
	Free_213,
	Free_214,
	Free_215,
	Free_216,
	Free_217,
	Free_218,
	Free_219,
	Free_220,
	Free_221,
	Free_222,
	Free_223,
	Free_224,
	Free_225,
	Free_226,
	Free_227,
	Free_228,
	Free_229,
	Free_230,
	Free_231,
	Free_232,
	Free_233,
	Free_234,
	Free_235,
	Free_236,
	Free_237,
	Free_238,
	Free_239,
	Free_240,
	Free_241,
	Free_242,
	Free_243,
	Free_244,
	Free_245,
	Free_246,
	Free_247,
	Free_248,
	Free_249,
	Free_250,
	Free_251,
	Free_252,
	Free_253,
	Free_254,
	Free_255
};

void* (* CustomReallocs[MAX_MODULES_TO_RECORD]) (void*, size_t) =
{
	Realloc_0,
	Realloc_1,
	Realloc_2,
	Realloc_3,
	Realloc_4,
	Realloc_5,
	Realloc_6,
	Realloc_7,
	Realloc_8,
	Realloc_9,
	Realloc_10,
	Realloc_11,
	Realloc_12,
	Realloc_13,
	Realloc_14,
	Realloc_15,
	Realloc_16,
	Realloc_17,
	Realloc_18,
	Realloc_19,
	Realloc_20,
	Realloc_21,
	Realloc_22,
	Realloc_23,
	Realloc_24,
	Realloc_25,
	Realloc_26,
	Realloc_27,
	Realloc_28,
	Realloc_29,
	Realloc_30,
	Realloc_31,
	Realloc_32,
	Realloc_33,
	Realloc_34,
	Realloc_35,
	Realloc_36,
	Realloc_37,
	Realloc_38,
	Realloc_39,
	Realloc_40,
	Realloc_41,
	Realloc_42,
	Realloc_43,
	Realloc_44,
	Realloc_45,
	Realloc_46,
	Realloc_47,
	Realloc_48,
	Realloc_49,
	Realloc_50,
	Realloc_51,
	Realloc_52,
	Realloc_53,
	Realloc_54,
	Realloc_55,
	Realloc_56,
	Realloc_57,
	Realloc_58,
	Realloc_59,
	Realloc_60,
	Realloc_61,
	Realloc_62,
	Realloc_63,
	Realloc_64,
	Realloc_65,
	Realloc_66,
	Realloc_67,
	Realloc_68,
	Realloc_69,
	Realloc_70,
	Realloc_71,
	Realloc_72,
	Realloc_73,
	Realloc_74,
	Realloc_75,
	Realloc_76,
	Realloc_77,
	Realloc_78,
	Realloc_79,
	Realloc_80,
	Realloc_81,
	Realloc_82,
	Realloc_83,
	Realloc_84,
	Realloc_85,
	Realloc_86,
	Realloc_87,
	Realloc_88,
	Realloc_89,
	Realloc_90,
	Realloc_91,
	Realloc_92,
	Realloc_93,
	Realloc_94,
	Realloc_95,
	Realloc_96,
	Realloc_97,
	Realloc_98,
	Realloc_99,
	Realloc_100,
	Realloc_101,
	Realloc_102,
	Realloc_103,
	Realloc_104,
	Realloc_105,
	Realloc_106,
	Realloc_107,
	Realloc_108,
	Realloc_109,
	Realloc_110,
	Realloc_111,
	Realloc_112,
	Realloc_113,
	Realloc_114,
	Realloc_115,
	Realloc_116,
	Realloc_117,
	Realloc_118,
	Realloc_119,
	Realloc_120,
	Realloc_121,
	Realloc_122,
	Realloc_123,
	Realloc_124,
	Realloc_125,
	Realloc_126,
	Realloc_127,
	Realloc_128,
	Realloc_129,
	Realloc_130,
	Realloc_131,
	Realloc_132,
	Realloc_133,
	Realloc_134,
	Realloc_135,
	Realloc_136,
	Realloc_137,
	Realloc_138,
	Realloc_139,
	Realloc_140,
	Realloc_141,
	Realloc_142,
	Realloc_143,
	Realloc_144,
	Realloc_145,
	Realloc_146,
	Realloc_147,
	Realloc_148,
	Realloc_149,
	Realloc_150,
	Realloc_151,
	Realloc_152,
	Realloc_153,
	Realloc_154,
	Realloc_155,
	Realloc_156,
	Realloc_157,
	Realloc_158,
	Realloc_159,
	Realloc_160,
	Realloc_161,
	Realloc_162,
	Realloc_163,
	Realloc_164,
	Realloc_165,
	Realloc_166,
	Realloc_167,
	Realloc_168,
	Realloc_169,
	Realloc_170,
	Realloc_171,
	Realloc_172,
	Realloc_173,
	Realloc_174,
	Realloc_175,
	Realloc_176,
	Realloc_177,
	Realloc_178,
	Realloc_179,
	Realloc_180,
	Realloc_181,
	Realloc_182,
	Realloc_183,
	Realloc_184,
	Realloc_185,
	Realloc_186,
	Realloc_187,
	Realloc_188,
	Realloc_189,
	Realloc_190,
	Realloc_191,
	Realloc_192,
	Realloc_193,
	Realloc_194,
	Realloc_195,
	Realloc_196,
	Realloc_197,
	Realloc_198,
	Realloc_199,
	Realloc_200,
	Realloc_201,
	Realloc_202,
	Realloc_203,
	Realloc_204,
	Realloc_205,
	Realloc_206,
	Realloc_207,
	Realloc_208,
	Realloc_209,
	Realloc_210,
	Realloc_211,
	Realloc_212,
	Realloc_213,
	Realloc_214,
	Realloc_215,
	Realloc_216,
	Realloc_217,
	Realloc_218,
	Realloc_219,
	Realloc_220,
	Realloc_221,
	Realloc_222,
	Realloc_223,
	Realloc_224,
	Realloc_225,
	Realloc_226,
	Realloc_227,
	Realloc_228,
	Realloc_229,
	Realloc_230,
	Realloc_231,
	Realloc_232,
	Realloc_233,
	Realloc_234,
	Realloc_235,
	Realloc_236,
	Realloc_237,
	Realloc_238,
	Realloc_239,
	Realloc_240,
	Realloc_241,
	Realloc_242,
	Realloc_243,
	Realloc_244,
	Realloc_245,
	Realloc_246,
	Realloc_247,
	Realloc_248,
	Realloc_249,
	Realloc_250,
	Realloc_251,
	Realloc_252,
	Realloc_253,
	Realloc_254,
	Realloc_255
};
