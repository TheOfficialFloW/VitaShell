#ifndef _PSP2_SHACCCG_H
#define _PSP2_SHACCCG_H

#ifdef	__cplusplus
extern "C" {
#endif	// def __cplusplus


typedef enum SceShaccCgDiagnosticLevel {
	SCE_SHACCCG_DIAGNOSTIC_LEVEL_INFO,				
	SCE_SHACCCG_DIAGNOSTIC_LEVEL_WARNING,			
	SCE_SHACCCG_DIAGNOSTIC_LEVEL_ERROR				
} SceShaccCgDiagnosticLevel;

typedef enum SceShaccCgTargetProfile {
	SCE_SHACCCG_PROFILE_VP,							
	SCE_SHACCCG_PROFILE_FP							
} SceShaccCgTargetProfile;

typedef enum SceShaccCgLocale {
	SCE_SHACCCG_ENGLISH,							
	SCE_SHACCCG_JAPANESE							
} SceShaccCgLocale;


typedef struct SceShaccCgSourceFile {
	char *fileName;							
	char *program;							
	uint32_t size;								
} SceShaccCgSourceFile;


typedef struct SceShaccCgSourceLocation {
	const SceShaccCgSourceFile *file;				
	uint32_t lineNumber;							
	uint32_t columnNumber;						
} SceShaccCgSourceLocation;

typedef struct {
  int field_0;
  int field_4;
  int field_8;
  int field_C;
  int field_10;
  int field_14;
} shader_first;


typedef struct SceShaccCgCompileOptions {
	const char *mainSourceFile;						
	SceShaccCgTargetProfile targetProfile;		
	const char *entryFunctionName;			
	uint32_t searchPathCount;					
	const char* const *searchPaths;				
	uint32_t macroDefinitionCount;				
	const char* const *macroDefinitions;		
	uint32_t includeFileCount;					
	const char* const *includeFiles;			
	uint32_t suppressedWarningsCount;				
	const uint32_t *suppressedWarnings;			
	SceShaccCgLocale locale;						
	int32_t useFx;									
	int32_t noStdlib;							
	int32_t optimizationLevel;					
	int32_t useFastmath;							
	int32_t useFastprecision;					
	int32_t useFastint;							
	int32_t warningsAsErrors;					
	int32_t performanceWarnings;					
	int32_t warningLevel;							
	int32_t pedantic;								
	int32_t pedanticError;							
  int field_5C;
  int field_60;
  int field_64;
} SceShaccCgCompileOptions;



typedef struct SceShaccCgDiagnosticMessage {
	SceShaccCgDiagnosticLevel level;			
	uint32_t code;								
	const SceShaccCgSourceLocation *location;	
	const char *message;						
} SceShaccCgDiagnosticMessage;



typedef struct SceShaccCgCompileOutput {
	const uint8_t *programData;						
	uint32_t programSize;						
	int32_t diagnosticCount;					
	const SceShaccCgDiagnosticMessage *diagnostics;	
} SceShaccCgCompileOutput;


int sceShaccCgInitializeCompileOptions(
	SceShaccCgCompileOptions options);
  
SceShaccCgCompileOutput * sceShaccCgCompile(
	const SceShaccCgCompileOptions *options,
  const shader_first * shader_first,
	void *callbacks);

void sceShaccCgDestroyCompileOutput(
	SceShaccCgCompileOutput const *output);


int compileShader(char *file, int *infos, int *warnings, int *errors);

#ifdef	__cplusplus
}
#endif	/* __cplusplus */

#endif /* _PSP2_SHACCCG_H */
