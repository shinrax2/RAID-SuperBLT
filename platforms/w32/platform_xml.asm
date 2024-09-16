.data
	extern NFXNF : qword
	extern NFX : qword

.code
	node_from_xml_new proc
		jmp NFXNF
	node_from_xml_new endp

	do_xmlload_invoke proc
		jmp NFX
	do_xmlload_invoke endp
end
