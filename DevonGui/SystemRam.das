define RAM_VBase					0x80000
define DefaultStack					RAM_VBase					+ 256		
define RAM_Font						DefaultStack				+ 512		
define RAM_Writer_YOffset			RAM_Font					+ 0x1a8		
define RAM_Writer_XOffset			RAM_Writer_YOffset			+ 1			
define RAM_Writer_Width				RAM_Writer_XOffset			+ 1			
define RAM_SendKeyToWriter			RAM_Writer_Width			+ 1			
define RAM_SpecialKeyToggles		RAM_SendKeyToWriter			+ 1			
define RAM_Writer_CursorState		RAM_SpecialKeyToggles		+ 1			
define RAM_Writer_CursorTimer		RAM_Writer_CursorState		+ 1			
define RAM_Writer_CursorChar		RAM_Writer_CursorTimer		+ 1			
define RAM_Writer_LineWidth			RAM_Writer_CursorChar		+ 1			
define RAM_Writer_BkSav				RAM_Writer_LineWidth		+ 1			
define RAM_Writer_BkSavAdd			RAM_Writer_BkSav			+ 7	
define RAM_ConsoleString			RAM_Writer_BkSavAdd			+ 2
define RAM_ConsoleStringCsr			RAM_ConsoleString			+ 256
define RAM_ConsoleString_ByteSpace	RAM_ConsoleStringCsr		+ 2 

define RAM_FirstAvailable		RAM_ConsoleStringCsr		+ 2
