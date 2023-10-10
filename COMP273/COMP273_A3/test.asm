.data 
	message: .asciiz "Hi, everybody"
	a: .float 1.1
.text
	main:
		addi $a1, $zero, 50
		addi $a2, $zero, 100
		jal addNumbers  
		
		li $v0 1
		addi $a0, $v1, 0
		syscall
	
		li $v0, 10
		syscall
		
	#call printComplex to test that it works
	lwc1 $f12, JuliaC1 #a
	lwc1 $f13, JuliaC1+4 #b
	jal printComplex
	
	#print New Line
	jal printNewLine
	
	#Compute the square of JuliaC1
	lwc1 $f12, JuliaC1 #a
	lwc1 $f13, JuliaC1+4 #b
	lwc1 $f14, JuliaC1 #c
	lwc1 $f15, JuliaC1+4 #d
	jal multComplex
	
	#print the outcome
	mov.s $f12, $f0 #a
	mov.s $f13, $f1 #b
	jal printComplex
			
		
		
	displayMessage:
		li $v0, 4
		la $a0, message
		syscall
		
		jr $ra
		
	addNumbers:
		add $v1, $a1, $a2
		 
		jr $ra
		