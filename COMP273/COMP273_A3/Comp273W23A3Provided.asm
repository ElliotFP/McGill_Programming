# Elliot Forcier-Poirier : 260989602
.data
bitmapDisplay: .space 0x80000 # enough memory for a 512x256 bitmap display
resolution: .word  512 256    # width and height of the bitmap display

windowlrbt: 
.float -2.5 2.5 -1.25 1.25  					# good window for viewing Julia sets
#.float -3 2 -1.25 1.25  					# good window for viewing full Mandelbrot set
#.float -0.807298 -0.799298 -0.179996 -0.175996 		# double spiral
#.float -1.019741354 -1.013877846  -0.325120847 -0.322189093 	# baby Mandelbrot
 
bound: .float 100	# bound for testing for unbounded growth during iteration
maxIter: .word 64	# maximum iteration count to be used by drawJulia and drawMandelbrot
scale: .word 80	# scale parameter used by computeColour

# Julia constants for testing, or likewise for more examples see
# https://en.wikipedia.org/wiki/Julia_set#Quadratic_polynomials  
JuliaC0:  .float 0    0    # should give you a circle, a good test, though boring!
JuliaC1:  .float 0.25 0.5 
JuliaC2:  .float 0    0.7 
JuliaC3:  .float 0    0.8 

# a demo starting point for iteration tests
x0: .float 1.0
y0: .float 0
z0: .float  0 0

two: .float 2.0
x: .asciiz "x"
y: .asciiz "y"
plus: .asciiz "+"
nl: .asciiz "\n"
i: .asciiz "i"
eq: .asciiz "="

# TODO: defiz ne various constants you need in your .data segment here

########################################################################################
.text
# TODO: Write your function testing code here
.main

	#call printComplex to test that it works
	lwc1 $f12, JuliaC1 #a
	lwc1 $f13, JuliaC1+4 #b
	jal printComplex
	
	#print New Line
	jal printNewLine
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

	jal printNewLine
	jal printNewLine
	
	#test the iterateVerbose function
	li $a1, 10 #n iterations
	lwc1 $f12, JuliaC1 #a 
	lwc1 $f13, JuliaC1+4 #b
	lwc1 $f14, z0 #x0
	lwc1 $f15, z0+4 #y0
	jal iterateVerbose 
	
	jal printNewLine
	jal printNewLine
	
	#second test
	li $a1, 10 #n iterations
	lwc1 $f12, JuliaC1 #a 
	lwc1 $f13, JuliaC1+4 #b
	lwc1 $f14, x0 #x0
	lwc1 $f15, y0 #y0
	jal iterateVerbose 
	
	jal printNewLine
	jal printNewLine	
	
	#Testing pixel2ComplexInWindow
	#Test with col=0,row=0	
	li $a0, 0
	li $a1, 0
	jal pixel2ComplexInWindow
	
	#print the outcome 
	mov.s $f12, $f0
	mov.s $f13, $f1
	jal printComplex
	
	jal printNewLine
	jal printNewLine
	
	#Test with col=256,row=128	
	li $a0, 256
	li $a1, 128
	jal pixel2ComplexInWindow
	
	#print the outcome 
	mov.s $f12, $f0
	mov.s $f13, $f1
	jal printComplex
	
	jal printNewLine
	jal printNewLine
	
	#Test with col=512,row=256	
	li $a0, 512
	li $a1, 256
	jal pixel2ComplexInWindow
	
	#print the outcome 
	mov.s $f12, $f0
	mov.s $f13, $f1
	jal printComplex
	
	jal printNewLine
	jal printNewLine
	
	#Test with col=512,row=0	
	li $a0, 512
	li $a1, 0
	jal pixel2ComplexInWindow
	
	#print the outcome 
	mov.s $f12, $f0
	mov.s $f13, $f1
	jal printComplex
	
	jal printNewLine
	jal printNewLine
	
	#Test with col=200,row=200	
	li $a0, 200
	li $a1, 200
	jal pixel2ComplexInWindow
	
	#print the outcome 
	mov.s $f12, $f0
	mov.s $f13, $f1
	jal printComplex
	
	jal printNewLine
	jal printNewLine
	
	#Test with col=100,row=100	
	li $a0, 100
	li $a1, 100
	jal pixel2ComplexInWindow
	
	#print the outcome 
	mov.s $f12, $f0
	mov.s $f13, $f1
	jal printComplex
	
	jal printNewLine
	jal printNewLine
	
	#Test with col=320,row=128	
	li $a0, 320
	li $a1, 128
	jal pixel2ComplexInWindow
	
	#print the outcome 
	mov.s $f12, $f0
	mov.s $f13, $f1
	jal printComplex
	
	jal printNewLine
	jal printNewLine
	
	#Test with col=192,row=128	
	li $a0, 192
	li $a1, 128
	jal pixel2ComplexInWindow
	
	#print the outcome 
	mov.s $f12, $f0
	mov.s $f13, $f1
	jal printComplex
	
	jal printNewLine
	jal printNewLine
	
	#call printComplex to test that it works
	lwc1 $f12, JuliaC1 #a
	lwc1 $f13, JuliaC1+4 #b
	jal drawJulia
	
	li $v0, 10
	syscall
# TODO: Write your functions to implement various assignment objectives here

#printComplex function which takes two float arguments $f12 and $f13 and prints on the ouput console "$f12 + $f13 i"
printComplex:
	# use stack
	addi $sp, $sp, -8
	swc1 $f12, 0($sp)
	swc1 $f13, 4($sp)
			
	#print a, real coefficient
	li $v0, 2
	syscall
	
	#print "+"
	li $v0, 4
	la $a0, plus
	syscall
	
	#print b, complex coefficient
	li $v0, 2
	mov.s $f12 $f13
	syscall
	
	#print "i"
	li $v0, 4
	la $a0, i
	syscall
	
	lwc1 $f12, 0($sp)
	lwc1 $f13, 4($sp)
	addi $sp, $sp, 8
	
	jr $ra
	
#printNewLine function which prints a newline character (\n)
printNewLine:
	#Print out "\n"
	li $v0, 4
	la $a0, nl
	syscall
	
	jr $ra
	
#multComplex, a function for multiplying two complex numbers, takes 4 inputs, a,b,c,d such that a+bi and c+di
multComplex:
	# Use stack
	addi $sp, $sp, -16
	swc1 $f12, 0($sp)
	swc1 $f13, 4($sp)
	swc1 $f14, 8($sp)
	swc1 $f15, 12($sp)
	
	# Process the necessary multiplications; ad,ac,bc,bd
	mul.s $f4, $f12, $f14
	mul.s $f5, $f13, $f15
	mul.s $f6, $f13, $f14
	mul.s $f7, $f12, $f15
	
	
	#Process the addition and subtraction 
	sub.s $f0 $f4 $f5
	add.s $f1 $f6 $f7
	
	
	#restore values
	lwc1 $f12, 0($sp)
	lwc1 $f13, 4($sp)
	lwc1 $f12, 8($sp)
	lwc1 $f13, 12($sp)
	addi $sp, $sp, 16
	
	jr $ra
	
	
#iterate verbose function
iterateVerbose:
	#set k=0 and go to while loop
	li $s0, 0
	addi $s1, $a1, -1 # we want strictly greater then not greater or equal
	
	#check the first number does not exceed the bound
	#load values onto the stack
	addi $sp, $sp, -20
	swc1 $f12, 0($sp) #a
	swc1 $f13, 4($sp) #b
	swc1 $f14, 8($sp) #x0
	swc1 $f15, 12($sp) #y0
	sw $ra, 16($sp) #store return value
		
	#Compute x^2 and y^2
	#x^2, basically (x +0i)(x+0i)
	mov.s $f12, $f14 #x
	sub.s $f13, $f13, $f13 
	mov.s $f14, $f14 #x
	sub.s $f15, $f15, $f15
	jal multComplex
		
	#save x^2
	mov.s $f20, $f0 #$f20 <- x^2
	
	#load from the stack again
	lwc1 $f12, 0($sp) #a
	lwc1 $f13, 4($sp) #b
	lwc1 $f14, 8($sp) #x0
	lwc1 $f15, 12($sp) #y0
		
	#y^2, same as x^2 but with y
	mov.s $f12, $f15 #y
	sub.s $f13, $f13, $f13 
	mov.s $f14, $f15 #y
	sub.s $f15, $f15, $f15 
	jal multComplex
		
	mov.s $f21, $f0 #$f21 <- y^2  
	
	#Compute x^2 + y^2
	add.s $f22, $f21, $f20
		
	#Check if bound < x^2 + y^2
	lwc1 $f23, bound
	c.lt.s $f23, $f22
	bc1t exit_1 
		
	#load from the stack again
	lwc1 $f12, 0($sp) #a
	lwc1 $f13, 4($sp) #b
	lwc1 $f14, 8($sp) #x0
	lwc1 $f15, 12($sp) #y0

	#print "x 0 + y 0 i =" for kth iteration
	#print x
	li $v0, 4
	la $a0, x
	syscall
	
	#print n for nth iteration
	li $v0, 1
	addi $a0, $s0, 0
	syscall
	
	#print "+"
	li $v0, 4
	la $a0, plus
	syscall
	
	#print "y"
	li $v0, 4
	la $a0, y
	syscall
	
	#print n for nth iteration
	li $v0, 1
	addi $a0, $s0, 0
	syscall
	
	#print "i"
	li $v0, 4
	la $a0, i
	syscall
	
	#print "="
	li $v0, 4
	la $a0, eq
	syscall
		
	#print the kth complex number
	mov.s $f12, $f14 #x0
	mov.s $f13, $f15 #y0
	jal printComplex
	jal printNewLine
	
	#check if k<n
	bgt $s0, $s1, exit_1
	
	#Calculate x_n+1
	#load from the stack again
	lwc1 $f12, 0($sp) #a
	lwc1 $f13, 4($sp) #b
	lwc1 $f14, 8($sp) #x0
	lwc1 $f15, 12($sp) #y0

	#the x^2 + a part
	add.s $f24, $f20, $f12
		
	#subtract y^2 from x^2 + a
	sub.s $f24, $f24, $f21 #f24 has x_n+1
		
	#Calculate y_n+1
	#load from the stack again
	lwc1 $f12, 0($sp) #a
	lwc1 $f13, 4($sp) #b
	lwc1 $f14, 8($sp) #x0
	lwc1 $f15, 12($sp) #y0
		
	#xy
	mov.s $f12, $f14 #x
	sub.s $f13, $f13, $f13 #0
	mov.s $f14, $f15 #y
	sub.s $f15, $f15, $f15 #0
	jal multComplex
		
	#load from the stack again
	lwc1 $f13, 4($sp) #b
	
	#the 2xy + b
	lwc1 $f25, two 
	mul.s $f25, $f0, $f25 #2 . (xy)
	add.s $f25, $f25, $f13 # + b
	#$f24 has y_n+1
		
	#load from the stack and the new values
	lwc1 $f12, 0($sp) #a
	lwc1 $f13, 4($sp) #b
	mov.s $f14, $f24#x0
	mov.s $f15, $f25 #y0
	lw $ra, 16($sp) #load the old return address from the stack
	addi $sp, $sp, 20
		
	j while_1
	
	#Iterate over the functions as long as k<n.
	while_1:
		#load values onto the stack
		addi $sp, $sp, -20
		swc1 $f12, 0($sp) #a
		swc1 $f13, 4($sp) #b
		swc1 $f14, 8($sp) #x0
		swc1 $f15, 12($sp) #y0
		sw $ra, 16($sp) #store return value
		
		#Compute x^2 and y^2
		#x^2, basically (x +0i)(x+0i)
		mov.s $f12, $f14 #x
		sub.s $f13, $f13, $f13 
		mov.s $f14, $f14 #x
		sub.s $f15, $f15, $f15
		jal multComplex
		
		#save x^2
		mov.s $f20, $f0 #$f20 <- x^2
	
		#load from the stack again
		lwc1 $f12, 0($sp) #a
		lwc1 $f13, 4($sp) #b
		lwc1 $f14, 8($sp) #x0
		lwc1 $f15, 12($sp) #y0
		
		#y^2, same as x^2 but with y
		mov.s $f12, $f15 #y
		sub.s $f13, $f13, $f13 
		mov.s $f14, $f15 #y
		sub.s $f15, $f15, $f15 
		jal multComplex
		
		mov.s $f21, $f0 #$f21 <- y^2  
	
		#Compute x^2 + y^2
		add.s $f22, $f21, $f20
		
		#Check if bound < x^2 + y^2
		lwc1 $f23, bound
		c.lt.s $f23, $f22
		bc1t exit_1 
		
		# Increment k by 1
		addi $s0, $s0, 1
		
		#check if k<n
		bgt $s0, $s1, exit_1
		
		#load from the stack again
		lwc1 $f12, 0($sp) #a
		lwc1 $f13, 4($sp) #b
		lwc1 $f14, 8($sp) #x0
		lwc1 $f15, 12($sp) #y0

		#print "x k + y k i =" for kth iteration
		#print x
		li $v0, 4
		la $a0, x
		syscall
	
		#print k for kth iteration
		li $v0, 1
		addi $a0, $s0, 0
		syscall
	
		#print "+"
		li $v0, 4
		la $a0, plus
		syscall
	
		#print "y"
		li $v0, 4
		la $a0, y
		syscall
		
		#print k for kth iteration
		li $v0, 1
		addi $a0, $s0, 0
		syscall
		
		#print "i"
		li $v0, 4
		la $a0, i
		syscall
		
		#print "="
		li $v0, 4
		la $a0, eq
		syscall
		
		#print the kth complex number
		mov.s $f12, $f14 #x0
		mov.s $f13, $f15 #y0
		jal printComplex
		jal printNewLine
		
		#Calculate x_n+1
		#load from the stack again
		lwc1 $f12, 0($sp) #a
		lwc1 $f13, 4($sp) #b
		lwc1 $f14, 8($sp) #x0
		lwc1 $f15, 12($sp) #y0

		#the x^2 + a part
		add.s $f23, $f20, $f12
		
		#subtract y^2 from x^2 + a
		sub.s $f23, $f23, $f0 #f23 has x_n+1
		
		#Calculate y_n+1
		#load from the stack again
		lwc1 $f12, 0($sp) #a
		lwc1 $f13, 4($sp) #b
		lwc1 $f14, 8($sp) #x0
		lwc1 $f15, 12($sp) #y0
		
		#xy
		mov.s $f12, $f14 #x
		sub.s $f13, $f13, $f13 #0
		mov.s $f14, $f15 #y
		sub.s $f15, $f15, $f15 #0
		jal multComplex
		
		#load from the stack again
		lwc1 $f13, 4($sp) #b
		
		#the 2xy + b
		lwc1 $f24, two 
		mul.s $f24, $f0, $f24 #2 . (xy)
		add.s $f24, $f24, $f13 # + b
		#$f24 has y_n+1
		
		#load from the stack and the new values
		lwc1 $f12, 0($sp) #a
		lwc1 $f13, 4($sp) #b
		mov.s $f14, $f23#x0
		mov.s $f15, $f24 #y0
		lw $ra, 16($sp) #load the old return address from the stack
		addi $sp, $sp, 20
		
		#loop
		j while_1
	exit_1: 		
		#restore values
		lwc1 $f12, 0($sp)
		lwc1 $f13, 4($sp)
		lwc1 $f14, 8($sp)
		lwc1 $f15, 12($sp)
		lw $ra, 16($sp)
		addi $sp, $sp, 20
		
		#print the number of iterations
		li $v0, 1
		addi $a0, $s0, 0
		syscall
		
		#set $v0 as the number of iterations
		addi $v0, $s0, 0
		jr $ra

iterate:
	#set k=0 
	li $s0, 0
	addi $s1, $a1, -1 # we want strictly greater then not greater or equal
	
	#check the first number does not exceed the bound
	#load values onto the stack
	addi $sp, $sp, -20
	swc1 $f12, 0($sp) #a
	swc1 $f13, 4($sp) #b
	swc1 $f14, 8($sp) #x0
	swc1 $f15, 12($sp) #y0
	sw $ra, 16($sp) #store return value
		
	#Compute x^2 and y^2
	#x^2, basically (x +0i)(x+0i)
	mov.s $f12, $f14 #x
	sub.s $f13, $f13, $f13 
	mov.s $f14, $f14 #x
	sub.s $f15, $f15, $f15
	jal multComplex
		
	#save x^2
	mov.s $f20, $f0 #$f20 <- x^2
	
	#load from the stack again
	lwc1 $f12, 0($sp) #a
	lwc1 $f13, 4($sp) #b
	lwc1 $f14, 8($sp) #x0
	lwc1 $f15, 12($sp) #y0
		
	#y^2, same as x^2 but with y
	mov.s $f12, $f15 #y
	sub.s $f13, $f13, $f13 
	mov.s $f14, $f15 #y
	sub.s $f15, $f15, $f15 
	jal multComplex
		
	mov.s $f21, $f0 #$f21 <- y^2  
	
	#Compute x^2 + y^2
	add.s $f22, $f20, $f21
		
	#Check if bound < x^2 + y^2
	lwc1 $f23, bound
	c.lt.s $f23, $f22
	bc1t exit_2 
		
	#load from the stack again
	lwc1 $f12, 0($sp) #a
	lwc1 $f13, 4($sp) #b
	lwc1 $f14, 8($sp) #x0
	lwc1 $f15, 12($sp) #y0
		
	#check if k<n
	bgt $s0, $s1, exit_2
		
	#Calculate x_n+1
	#load from the stack again
	lwc1 $f12, 0($sp) #a
	lwc1 $f13, 4($sp) #b
	lwc1 $f14, 8($sp) #x0
	lwc1 $f15, 12($sp) #y0

	#the x^2 + a part
	add.s $f24, $f20, $f12
		
	#subtract y^2 from x^2 + a
	sub.s $f24, $f24, $f21 #f24 has x_n+1
		
	#Calculate y_n+1
	#load from the stack again
	lwc1 $f12, 0($sp) #a
	lwc1 $f13, 4($sp) #b
	lwc1 $f14, 8($sp) #x0
	lwc1 $f15, 12($sp) #y0
		
	#xy
	mov.s $f12, $f14 #x
	sub.s $f13, $f13, $f13 #0
	mov.s $f14, $f15 #y
	sub.s $f15, $f15, $f15 #0
	jal multComplex
		
	#load from the stack again
	lwc1 $f13, 4($sp) #b
	
	#the 2xy + b
	lwc1 $f25, two 
	mul.s $f25, $f0, $f25 #2 . (xy)
	add.s $f25, $f25, $f13 # + b
	#$f24 has y_n+1
		
	#load from the stack and the new values
	lwc1 $f12, 0($sp) #a
	lwc1 $f13, 4($sp) #b
	mov.s $f14, $f24#x0
	mov.s $f15, $f25 #y0
	lw $ra, 16($sp) #load the old return address from the stack
	addi $sp, $sp, 20
		
	j while_2
	
	#Iterate over the functions as long as k<n.
	while_2:
		#load values onto the stack
		addi $sp, $sp, -20
		swc1 $f12, 0($sp) #a
		swc1 $f13, 4($sp) #b
		swc1 $f14, 8($sp) #x0
		swc1 $f15, 12($sp) #y0
		sw $ra, 16($sp) #store return value
		
		#Compute x^2 and y^2
		#x^2, basically (x +0i)(x+0i)
		mov.s $f12, $f14 #x
		sub.s $f13, $f13, $f13 
		mov.s $f14, $f14 #x
		sub.s $f15, $f15, $f15
		jal multComplex
		
		#save x^2
		mov.s $f20, $f0 #$f20 <- x^2
	
		#load from the stack again
		lwc1 $f12, 0($sp) #a
		lwc1 $f13, 4($sp) #b
		lwc1 $f14, 8($sp) #x0
		lwc1 $f15, 12($sp) #y0
		
		#y^2, same as x^2 but with y
		mov.s $f12, $f15 #y
		sub.s $f13, $f13, $f13 
		mov.s $f14, $f15 #y
		sub.s $f15, $f15, $f15 
		jal multComplex
		
		mov.s $f21, $f0 #$f21 <- y^2  
	
		#Compute x^2 + y^2
		add.s $f22, $f21, $f20
		
		#Check if bound < x^2 + y^2
		lwc1 $f23, bound
		c.lt.s $f23, $f22
		bc1t exit_2 
		
		# Increment k by 1
		addi $s0, $s0, 1
		
		#check if k<n
		bgt $s0, $s1, exit_2
		
		#load from the stack again
		lwc1 $f12, 0($sp) #a
		lwc1 $f13, 4($sp) #b
		lwc1 $f14, 8($sp) #x0
		lwc1 $f15, 12($sp) #y0
		
		#Calculate x_n+1
		#load from the stack again
		lwc1 $f12, 0($sp) #a
		lwc1 $f13, 4($sp) #b
		lwc1 $f14, 8($sp) #x0
		lwc1 $f15, 12($sp) #y0

		#the x^2 + a part
		add.s $f24, $f20, $f12
		
		#subtract y^2 from x^2 + a
		sub.s $f24, $f24, $f21 #f24 has x_n+1
		
		#Calculate y_n+1
		#load from the stack again
		lwc1 $f12, 0($sp) #a
		lwc1 $f13, 4($sp) #b
		lwc1 $f14, 8($sp) #x0
		lwc1 $f15, 12($sp) #y0
		
		#xy
		mov.s $f12, $f14 #x
		sub.s $f13, $f13, $f13 #0
		mov.s $f14, $f15 #y
		sub.s $f15, $f15, $f15 #0
		jal multComplex
		
		#load from the stack again
		lwc1 $f13, 4($sp) #b
		
		#the 2xy + b
		lwc1 $f25, two 
		mul.s $f25, $f25, $f0 #2 . (xy)
		add.s $f25, $f25, $f13 # + b
		#$f25 has y_n+1
		
		#load from the stack and the new values
		lwc1 $f12, 0($sp) #a
		lwc1 $f13, 4($sp) #b
		mov.s $f14, $f24#x_n+1
		mov.s $f15, $f25 #y_n+1
		lw $ra, 16($sp) #load the old return address from the stack
		addi $sp, $sp, 20
		
		#loop
		j while_2
	exit_2: 		
		#restore values
		lwc1 $f12, 0($sp)
		lwc1 $f13, 4($sp)
		lwc1 $f14, 8($sp)
		lwc1 $f15, 12($sp)
		lw $ra, 16($sp)
		addi $sp, $sp, 20
		
		#set $v0 as the number of iterations
		addi $v0, $s0, 0
		jr $ra
	
pixel2ComplexInWindow:
	#Convert ints to floats
	mtc1 $a0, $f4 #$f4 contains col
	cvt.s.w $f4, $f4
	mtc1 $a1, $f5 #$f5 contains row
	cvt.s.w $f5, $f5
	
	#Convert words to floats
	l.s $f6, resolution
	l.s $f7, resolution+4
	cvt.s.w $f8, $f6 #$f8 contains width
	cvt.s.w $f9, $f7 #$f9 contains height
	
	#load l,r,b,t
	lwc1 $f10, windowlrbt #l
	lwc1 $f11, windowlrbt+4 #r
	lwc1 $f12, windowlrbt+8 #b
	lwc1 $f13, windowlrbt+12 #t
			
	#compute x
	div.s $f14, $f4, $f8 #compute col/w
	sub.s $f15, $f11, $f10 #compute (r-l)
	mul.s $f14, $f14, $f15 #compute col/w (r-l)
	add.s $f14, $f14, $f10 #compute col/w (r-l) + l	
	
	#save x as a return value
	mov.s $f0, $f14
	
	#compute x
	div.s $f14, $f5, $f9 #compute row/h
	sub.s $f15, $f13, $f12 #compute (t-b)
	mul.s $f14, $f14, $f15 #compute row/h (t-b)
	add.s $f14, $f14, $f12 #compute row/h (t-b) + b	
	
	#save y as a return value
	mov.s $f1, $f14

	jr $ra
	
drawJulia:
	#Store inputs as well as return value
	addi $sp, $sp, -12
	swc1 $f12, 0($sp)
	swc1 $f13, 4($sp)
	sw $ra, 8($sp)
	
	#load address of first pixel
	la $s6, bitmapDisplay
	
	#set k=0 and go to while loop
	li $s2, 0
	lw $s3, resolution+4 #h
	addi $s3, $s3, -1 #we want 256 iterations, not 257
	
	j while_3
	
	#this while loop iterates over all columns
	while_3:
		#check that k<w
		bgt $s2, $s3, exit_3
		
		#set j=0 and go to while loop
		li $s4, 0
		lw $s5, resolution #w
		addi $s5, $s5, -1 #we want 512 iterations, not 513
		
		j while_4
		
		#this while loop iterates over all rows
		while_4:
			#Check that j<h
			bgt $s4, $s5, exit_4
			
			#call pixel2ComplexInWindow on the current column $s2, row $s4
			addi $a0, $s4, 0 #col
			addi $a1, $s2, 0 #row
			jal pixel2ComplexInWindow
									
			#call iterate on the given 
			lw $a1, maxIter #n iterations
			lwc1 $f12, 0($sp) #a
			lwc1 $f13, 4($sp) #b
			mov.s $f14, $f0 #x
			mov.s $f15, $f1 #y
			jal iterate 
			
			#save the return value
			addi $t0, $v0, 0
			lw $t1, maxIter
			
			#check if the we exceeded the bound
			beq $t1, $t0, InJulia
				#get the colour
				addi $a0, $t0, 0
				jal computeColour
				
				addi $t0, $v0, 0
				
				j endPixelLoop
				
			
			#what to do when point is in the julia set, i.e. should be black
			InJulia:
				#set colour to black
				li $t0, 0
				j endPixelLoop
				
			#End the loop of this specific pixel	
			endPixelLoop:
				#store the pixel's colour value at the appropriate location	
				sw $t0, ($s6)	
				
				#increment pointer to go to the next pixel
				addi $s6, $s6, 4
				
				#increment the col
				addi $s4, $s4, 1
				
				#Reload the original values
				j while_4
			
			
		exit_4:
			#increment the row
			addi $s2, $s2, 1
			
			j while_3
		
		
	exit_3:
		#Load values and return value
		lwc1 $f12, 0($sp)
		lwc1 $f13, 4($sp)
		lw $ra, 8($sp)
		addi $sp, $sp, 12
		
		jr $ra
		
drawMandelbrot:
	#Store inputs as well as return value
	addi $sp, $sp, -4
	sw $ra, 0($sp)
	
	#load address of first pixel
	la $s6, bitmapDisplay
	
	#set k=0 and go to while loop
	li $s2, 0
	lw $s3, resolution+4 #h
	addi $s3, $s3, -1 #we want 256 iterations, not 257
	
	j while_5
	
	#this while loop iterates over all columns
	while_5:
		#check that k<w
		bgt $s2, $s3, exit_5
		
		#set j=0 and go to while loop
		li $s4, 0
		lw $s5, resolution #w
		addi $s5, $s5, -1 #we want 512 iterations, not 513
		
		j while_6
		
		#this while loop iterates over all rows
		while_6:
			#Check that j<h
			bgt $s4, $s5, exit_6
			
			#call pixel2ComplexInWindow on the current column $s2, row $s4
			addi $a0, $s4, 0 #col
			addi $a1, $s2, 0 #row
			jal pixel2ComplexInWindow
									
			#call iterate on the given 
			lw $a1, maxIter #n iterations
			mov.s $f12, $f0 #a
			mov.s $f13, $f1 #b
			lwc1 $f14, z0 #x = 0
			lwc1 $f15, z0+4 #y = 0
			jal iterate 
			
			#save the return value
			addi $t0, $v0, 0
			lw $t1, maxIter
			
			#check if the we exceeded the bound
			beq $t1, $t0, InMandel
				#get the colour
				addi $a0, $t0, 0
				jal computeColour
				
				addi $t0, $v0, 0
				
				j endPixelLoop_2
				
			
			#what to do when point is in the julia set, i.e. should be black
			InMandel:
				#set colour to black
				li $t0, 0
				j endPixelLoop_2
				
			#End the loop of this specific pixel	
			endPixelLoop_2:
				#store the pixel's colour value at the appropriate location	
				sw $t0, ($s6)	
				
				#increment pointer to go to the next pixel
				addi $s6, $s6, 4
				
				#increment the col
				addi $s4, $s4, 1
				
				#Reload the original values
				j while_6
			
			
		exit_6:
			#increment the row
			addi $s2, $s2, 1
			
			j while_5
		
		
	exit_5:
		#Load values and return value
		lw $ra, 0($sp)
		addi $sp, $sp, 4
		
		jr $ra
	
		
	
	
		
	



########################################################################################
# Computes a colour corresponding to a given iteration count in $a0
# The colours cycle smoothly through green blue and red, with a speed adjustable 
# by a scale parametre defined in the static .data segment
computeColour:
	la $t0 scale
	lw $t0 ($t0)
	mult $a0 $t0
	mflo $a0
ccLoop:
	slti $t0 $a0 256
	beq $t0 $0 ccSkip1
	li $t1 255
	sub $t1 $t1 $a0
	sll $t1 $t1 8
	add $v0 $t1 $a0
	jr $ra
ccSkip1:
  	slti $t0 $a0 512
	beq $t0 $0 ccSkip2
	addi $v0 $a0 -256
	li $t1 255
	sub $t1 $t1 $v0
	sll $v0 $v0 16
	or $v0 $v0 $t1
	jr $ra
ccSkip2:
	slti $t0 $a0 768
	beq $t0 $0 ccSkip3
	addi $v0 $a0 -512
	li $t1 255
	sub $t1 $t1 $v0
	sll $t1 $t1 16
	sll $v0 $v0 8
	or $v0 $v0 $t1
	jr $ra
ccSkip3:
 	addi $a0 $a0 -768
 	j ccLoop
