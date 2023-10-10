# Forcier-Poirier
# Elliot
# 260989602

# Questions 1 and 2

# Q1: Yes, because cache is 128 bytes (0x80) and each buffer is 0x40000 bytes (2048 x size of cache). So,
# the address of the error buffer is the adress of image buffer + (2048 * size of cache). Which means they have 
# the same size as well as offset, which is why their pixels collide in the cache and we have to use a dummy space in the fast implementation.

# Q2: No, because there will always be pixels of the error and image buffer mapped to the same 
# block. This happens since the cache has a much smaller size either buffer. So, there's no real speed improvement. Which is why we add a dummy space.

.data
displayBuffer:  .space 0x40000 # space for 512x256 bitmap display 
#dummyBuffer: .space 0x40
errorBuffer:    .space 0x40000 # space to store match function
templateBuffer: .space 0x100   # space for 8x8 template
imageFileName:    .asciiz "pxlcon512x256cropgs.raw" 
templateFileName: .asciiz "template8x8gsJoker.raw"
# struct bufferInfo { int *buffer, int width, int height, char* filename }
imageBufferInfo:    .word displayBuffer  512 128  imageFileName
errorBufferInfo:    .word errorBuffer    512 128  0
templateBufferInfo: .word templateBuffer 8   8    templateFileName
nl: .asciiz "\n"


.text
main:	
	la $a0, imageBufferInfo
	jal loadImage
	la $a0, templateBufferInfo
	jal loadImage
	la $a0, imageBufferInfo
	la $a1, templateBufferInfo
	la $a2, errorBufferInfo
	jal matchTemplateFast 
	la $a0, errorBufferInfo
	jal findBest
	la $a0, imageBufferInfo
	move $a1, $v0
	jal highlight
	la $a0, errorBufferInfo	
	jal processError
	li $v0, 10		# exit
	syscall
	
##########################################################
# matchTemplate( bufferInfo imageBufferInfo, bufferInfo templateBufferInfo, bufferInfo errorBufferInfo )
# NOTE: struct bufferInfo { int *buffer, int width, int height, char* filename }
matchTemplate:
	#load pointer onto stack
	addi $sp, $sp, -24
	sw $s0, 0($sp)
	sw $s1, 4($sp)
	sw $s2, 8($sp)
	sw $s3, 12($sp)
	sw $s4, 16($sp)
	sw $s5, 20($sp)
	
	# load image parameters
	lw $s0, 0($a2) # errorBuffer pointer
	lw $s1, 0($a0) # displayBuffer pointer
	lw $s2, 0($a1) # templateBuffer pointer 
	lw $s3, 4($a0) # width
	lw $s4, 4($a1) # template width
	lw $s5, 8($a0) # height
	addi $s3, $s3, -8 
	addi $s5, $s5, -8 # assure there's a buffer the length of the template
	addi $s4, $s4, -1
	
	#set y = 0
	li $t6, 0
	
	for_y:
		#check if y > height - 8, if so, branch
		bgt $t6, $s5, exit_y
		
		#set x = 0
		li $t7, 0
		
		for_x:
			#Check that x > w - 8, if so, branch
			bgt $t7, $s3, exit_x
			
			#set i = 0
			li $t8, 0
			
			#initialize $v0 to 0, which is the error for our current (x,y)
			li $v0, 0
			
			# load templateBuffer and displayBuffer pointers into temps
			move $t1, $s1
			move $t2, $s2 #so we dont have to restore the pointers after each the for i loop
	
			for_i:
				#check if i >= 8, if so, branch
				bgt $t8, $s4, exit_i
	
				#set j = 0
				li $t9, 0
		
				for_j:
					#Check that j >= 8, if so, branch
					bgt $t9, $s4, exit_j
			
					#load brightness of the respective pixels
					lbu $t3, ($t1)
					lbu $t4, ($t2)
			
					#compute abs difference
					sub $t3, $t3, $t4
					abs $t3, $t3
			
					#add to cum total
					add $v0, $v0, $t3
			
					# increment pixel pointer
					addi $t1, $t1, 4
					addi $t2, $t2, 4
			
					#increment j
					addi $t9, $t9, 1
					
					j for_j
		
				exit_j:
					#increment i
					addi $t8, $t8, 1
	
					#send the displayBuffer pointer to the next row
					addi $t1, $t1, 2016
					
					#the templateBuffer does not need to be incremented since we are already at the next row

					j for_i
			exit_i:
				
				#put error into errorBuffer pixel address
				sw $v0, ($s0)
			
				# increment displayBuffer and errorBuffer pointer to go to next column
				addi $s0, $s0, 4
				addi $s1, $s1, 4
				
				# increment x
				addi $t7, $t7, 1
			
				j for_x #loop
		
		exit_x:
			# increment y
			addi $t6, $t6, 1
			
			# increment displayBuffer and errorBuffer pointers to skip to next row
			addi $s0, $s0, 28
			addi $s1, $s1, 28
			
			j for_y
		
	exit_y:
		#load from the stack and restore the saved registers
		lw $s0, 0($sp)
		lw $s1, 4($sp)
		lw $s2, 8($sp)
		lw $s3, 12($sp)
		lw $s4, 16($sp)
		lw $s5, 20($sp)
		addi $sp, $sp, 24
	
		jr $ra		
	
##########################################################
# matchTemplateFast( bufferInfo imageBufferInfo, bufferInfo templateBufferInfo, bufferInfo errorBufferInfo )
# NOTE: struct bufferInfo { int *buffer, int width, int height, char* filename }
matchTemplateFast:	
	addi $sp, $sp, -32
	sw $s0, ($sp)
	sw $s1, 4($sp)
	sw $s2, 8($sp)
	sw $s3, 12($sp)
	sw $s4, 16($sp)
	sw $s5, 20($sp)
	sw $s6, 24($sp)
	sw $ra, 28($sp)
		
	
	
	# load image parameters
	lw $s0, 0($a0) # displayBuffer pointer
	lw $s1, 0($a1) # templateBuffer pointer
	lw $s2, 0($a2) # errorBuffer pointer
	lw $s3, 4($a0) # image width
	lw $s4, 8($a0) # image height 
	
	#shenanigans to lower instruction count
	addi $s4, $s4, -8 # assure there's a buffer the length of the template 
	mul $s3, $s3, 4 # adapt it to save instructions
	mul $t0, $s3, $s4
	addi $s5, $s3, -32 # assure there's a buffer the length of the template 
	addi $v1, $s1, 256 # this is for performance reasons, this function is not supposed to return a value anyways 8 x 32 = 256
	add $s4, $s0, $t0 # address of first pixel of the 121st row
	
	# Unrolling the j loop
	 
		# store the brightness values of the jth row of the template
		lbu $t0, 0($s1) 
		lbu $t1, 4($s1)
		lbu $t2, 8($s1)
		lbu $t3, 12($s1)
		lbu $t4, 16($s1)
		lbu $t5, 20($s1)
		lbu $t6, 24($s1)
		lbu $t7, 28($s1)
		
		jal ffor_j
		
		addi $s1, $s1, 32
		
		# store the brightness values of the jth row of the template
		lbu $t0, 0($s1) 
		lbu $t1, 4($s1)
		lbu $t2, 8($s1)
		lbu $t3, 12($s1)
		lbu $t4, 16($s1)
		lbu $t5, 20($s1)
		lbu $t6, 24($s1)
		lbu $t7, 28($s1)
		
		add $s4, $s4, $s3 # Augment boundary
		add $s0, $s0, $s3 # Next row of display
		
		jal ffor_j
		
		addi $s1, $s1, 32
		
		# store the brightness values of the jth row of the template
		lbu $t0, 0($s1) 
		lbu $t1, 4($s1)
		lbu $t2, 8($s1)
		lbu $t3, 12($s1)
		lbu $t4, 16($s1)
		lbu $t5, 20($s1)
		lbu $t6, 24($s1)
		lbu $t7, 28($s1)
		
		add $s4, $s4, $s3 # Augment boundary
		add $s0, $s0, $s3 # Next row of display
		
		jal ffor_j
		
		addi $s1, $s1, 32
		
		# store the brightness values of the jth row of the template
		lbu $t0, 0($s1) 
		lbu $t1, 4($s1)
		lbu $t2, 8($s1)
		lbu $t3, 12($s1)
		lbu $t4, 16($s1)
		lbu $t5, 20($s1)
		lbu $t6, 24($s1)
		lbu $t7, 28($s1)
		
		add $s4, $s4, $s3 # Augment boundary
		add $s0, $s0, $s3 # Next row of display
		
		jal ffor_j
		
		addi $s1, $s1, 32
		
		# store the brightness values of the jth row of the template
		lbu $t0, 0($s1) 
		lbu $t1, 4($s1)
		lbu $t2, 8($s1)
		lbu $t3, 12($s1)
		lbu $t4, 16($s1)
		lbu $t5, 20($s1)
		lbu $t6, 24($s1)
		lbu $t7, 28($s1)
		
		add $s4, $s4, $s3 # Augment boundary
		add $s0, $s0, $s3 # Next row of display
		
		jal ffor_j
		
		addi $s1, $s1, 32
		
		# store the brightness values of the jth row of the template
		lbu $t0, 0($s1) 
		lbu $t1, 4($s1)
		lbu $t2, 8($s1)
		lbu $t3, 12($s1)
		lbu $t4, 16($s1)
		lbu $t5, 20($s1)
		lbu $t6, 24($s1)
		lbu $t7, 28($s1)
		
		add $s4, $s4, $s3 # Augment boundary
		add $s0, $s0, $s3 # Next row of display
		
		jal ffor_j
		
		addi $s1, $s1, 32
		
		# store the brightness values of the jth row of the template
		lbu $t0, 0($s1) 
		lbu $t1, 4($s1)
		lbu $t2, 8($s1)
		lbu $t3, 12($s1)
		lbu $t4, 16($s1)
		lbu $t5, 20($s1)
		lbu $t6, 24($s1)
		lbu $t7, 28($s1)
		
		add $s4, $s4, $s3 # Augment boundary
		add $s0, $s0, $s3 # Next row of display
		
		jal ffor_j
		
		addi $s1, $s1, 32
		
		# store the brightness values of the jth row of the template
		lbu $t0, 0($s1) 
		lbu $t1, 4($s1)
		lbu $t2, 8($s1)
		lbu $t3, 12($s1)
		lbu $t4, 16($s1)
		lbu $t5, 20($s1)
		lbu $t6, 24($s1)
		lbu $t7, 28($s1)
		
		add $s4, $s4, $s3 # Augment boundary
		add $s0, $s0, $s3 # Next row of display
		
		jal ffor_j
		
	# Load back from the stack
	lw $s0, 0($sp)
	lw $s1, 4($sp)
	lw $s2, 8($sp)
	lw $s3, 12($sp)
	lw $s4, 16($sp)
	lw $s5, 20($sp)
	lw $s6, 24($sp)
	lw $ra, 28($sp)
	addi $sp, $sp, 32
	
	
	jr $ra	
	
		
ffor_j:
	addi $sp, $sp, -16
	sw $s0, 0($sp)
	sw $s2, 4($sp)
	sw $s6, 8($sp)
	sw $ra, 12($sp)
	
	ffor_y:
		#check if address pointer > address of first pixel of 121st row, if so, branch
		bgt $s0, $s4, fexit_y	
			
		add $s6, $s0, $s5 # new address of 505th pixel of of our column
			
		ffor_x:
				#check if address pointer > address of 505th pixel of of our column, if so, branch
				bgt $s0, $s6, fexit_x	
				
				lw $t8, ($s2) # Get the sum of the SADs of the previous rows
				
				# Calculate abs between template and display at each of the 8 pixel in the row
				lbu $t9, 0($s0) 
				sub $t9, $t9, $t0
				abs $t9, $t9
				add $t8, $t8, $t9 # Add to total
		
				lbu $t9, 4($s0) 
				sub $t9, $t9, $t1
				abs $t9, $t9
				add $t8, $t8, $t9 
				
				lbu $t9, 8($s0) 
				sub $t9, $t9, $t2
				abs $t9, $t9
				add $t8, $t8, $t9
				
				lbu $t9, 12($s0) 
				sub $t9, $t9, $t3
				abs $t9, $t9
				add $t8, $t8, $t9 
				
				lbu $t9, 16($s0) 
				sub $t9, $t9, $t4
				abs $t9, $t9
				add $t8, $t8, $t9 
				
				lbu $t9, 20($s0) 
				sub $t9, $t9, $t5
				abs $t9, $t9
				add $t8, $t8, $t9 
				
				lbu $t9, 24($s0) 
				sub $t9, $t9, $t6
				abs $t9, $t9
				add $t8, $t8, $t9 
				
				lbu $t9, 28($s0) 
				sub $t9, $t9, $t7
				abs $t9, $t9
				add $t8, $t8, $t9 
				
				sw $t8,($s2) #Store the new total error into the corresponding errorBuffer pixel addres
				
				# Increment errorBuffer and displayBuffer pointers
				addi $s0, $s0, 4
				addi $s2, $s2, 4 
				
				j ffor_x
				
			fexit_x:
			#Get to next row for errorBuffer and displayBuffer pointers
			addi $s0, $s0, 28 # x is the first pixel in the new row
			addi $s2, $s2, 28 # errorBuffer
			
			j ffor_y
			
		fexit_y:
		
		#load back the saved registers we used from the stack
		lw $s0, 0($sp)
		lw $s2, 4($sp)
		lw $s6, 8($sp)
		lw $ra, 12($sp)
		addi $sp, $sp, 16
		
		jr $ra
			
	
###############################################################
# loadImage( bufferInfo* imageBufferInfo )
# NOTE: struct bufferInfo { int *buffer, int width, int height, char* filename }
loadImage:	lw $a3, 0($a0)  # int* buffer
		lw $a1, 4($a0)  # int width
		lw $a2, 8($a0)  # int height
		lw $a0, 12($a0) # char* filename
		mul $t0, $a1, $a2 # words to read (width x height) in a2
		sll $t0, $t0, 2	  # multiply by 4 to get bytes to read
		li $a1, 0     # flags (0: read, 1: write)
		li $a2, 0     # mode (unused)
		li $v0, 13    # open file, $a0 is null-terminated string of file name
		syscall
		move $a0, $v0     # file descriptor (negative if error) as argument for read
  		move $a1, $a3     # address of buffer to which to write
		move $a2, $t0	  # number of bytes to read
		li  $v0, 14       # system call for read from file
		syscall           # read from file
        		# $v0 contains number of characters read (0 if end-of-file, negative if error).
        		# We'll assume that we do not need to be checking for errors!
		# Note, the bitmap display doesn't update properly on load, 
		# so let's go touch each memory address to refresh it!
		move $t0, $a3	   # start address
		add $t1, $a3, $a2  # end address
loadloop:	lw $t2, ($t0)
		sw $t2, ($t0)
		addi $t0, $t0, 4
		bne $t0, $t1, loadloop
		jr $ra
		
		
#####################################################
# (offset, score) = findBest( bufferInfo errorBuffer )
# Returns the address offset and score of the best match in the error Buffer
findBest:	lw $t0, 0($a0)     # load error buffer start address	
		lw $t2, 4($a0)	   # load width
		lw $t3, 8($a0)	   # load height
		addi $t3, $t3, -7  # height less 8 template lines minus one
		mul $t1, $t2, $t3
		sll $t1, $t1, 2    # error buffer size in bytes	
		add $t1, $t0, $t1  # error buffer end address
		li $v0, 0		# address of best match	
		li $v1, 0xffffffff 	# score of best match	
		lw $a1, 4($a0)    # load width
        		addi $a1, $a1, -7 # initialize column count to 7 less than width to account for template
fbLoop:		lw $t9, 0($t0)        # score
		sltu $t8, $t9, $v1    # better than best so far?
		beq $t8, $zero, notBest
		move $v0, $t0
		move $v1, $t9
notBest:		addi $a1, $a1, -1
		bne $a1, $0, fbNotEOL # Need to skip 8 pixels at the end of each line
		lw $a1, 4($a0)        # load width
        		addi $a1, $a1, -7     # column count for next line is 7 less than width
        		addi $t0, $t0, 28     # skip pointer to end of line (7 pixels x 4 bytes)
fbNotEOL:	add $t0, $t0, 4
		bne $t0, $t1, fbLoop
		lw $t0, 0($a0)     # load error buffer start address	
		sub $v0, $v0, $t0  # return the offset rather than the address
		jr $ra
		

#####################################################
# highlight( bufferInfo imageBuffer, int offset )
# Applies green mask on all pixels in an 8x8 region
# starting at the provided addr.
highlight:	lw $t0, 0($a0)     # load image buffer start address
		add $a1, $a1, $t0  # add start address to offset
		lw $t0, 4($a0) 	# width
		sll $t0, $t0, 2	
		li $a2, 0xff00 	# highlight green
		li $t9, 8	# loop over rows
highlightLoop:	lw $t3, 0($a1)		# inner loop completely unrolled	
		and $t3, $t3, $a2
		sw $t3, 0($a1)
		lw $t3, 4($a1)
		and $t3, $t3, $a2
		sw $t3, 4($a1)
		lw $t3, 8($a1)
		and $t3, $t3, $a2
		sw $t3, 8($a1)
		lw $t3, 12($a1)
		and $t3, $t3, $a2
		sw $t3, 12($a1)
		lw $t3, 16($a1)
		and $t3, $t3, $a2
		sw $t3, 16($a1)
		lw $t3, 20($a1)
		and $t3, $t3, $a2
		sw $t3, 20($a1)
		lw $t3, 24($a1)
		and $t3, $t3, $a2
		sw $t3, 24($a1)
		lw $t3, 28($a1)
		and $t3, $t3, $a2
		sw $t3, 28($a1)
		add $a1, $a1, $t0	# increment address to next row	
		add $t9, $t9, -1		# decrement row count
		bne $t9, $zero, highlightLoop
		jr $ra

######################################################
# processError( bufferInfo error )
# Remaps scores in the entire error buffer. The best score, zero, 
# will be bright green (0xff), and errors bigger than 0x4000 will
# be black.  This is done by shifting the error by 5 bits, clamping
# anything bigger than 0xff and then subtracting this from 0xff.
processError:	lw $t0, 0($a0)     # load error buffer start address
		lw $t2, 4($a0)	   # load width
		lw $t3, 8($a0)	   # load height
		addi $t3, $t3, -7  # height less 8 template lines minus one
		mul $t1, $t2, $t3
		sll $t1, $t1, 2    # error buffer size in bytes	
		add $t1, $t0, $t1  # error buffer end address
		lw $a1, 4($a0)     # load width as column counter
        		addi $a1, $a1, -7  # initialize column count to 7 less than width to account for template
pebLoop:		lw $v0, 0($t0)        # score
		srl $v0, $v0, 5       # reduce magnitude 
		slti $t2, $v0, 0x100  # clamp?
		bne  $t2, $zero, skipClamp
		li $v0, 0xff          # clamp!
skipClamp:	li $t2, 0xff	      # invert to make a score
		sub $v0, $t2, $v0
		sll $v0, $v0, 8       # shift it up into the green
		sw $v0, 0($t0)
		addi $a1, $a1, -1        # decrement column counter	
		bne $a1, $0, pebNotEOL   # Need to skip 8 pixels at the end of each line
		lw $a1, 4($a0)        # load width to reset column counter
        		addi $a1, $a1, -7     # column count for next line is 7 less than width
        		addi $t0, $t0, 28     # skip pointer to end of line (7 pixels x 4 bytes)
pebNotEOL:	add $t0, $t0, 4
		bne $t0, $t1, pebLoop
		jr $ra
