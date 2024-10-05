.data
	extern PA : qword

.code
	jumpToPA proc
		jmp qword ptr [PA]
	jumpToPA endp
end
