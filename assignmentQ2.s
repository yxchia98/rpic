.data
.balign 4
input_integer_string:.asciz "\nEnter an integer number:"
input_option_string:.asciz "\nEnter the Option 1 or 2:"
input_format:.asciz "%d"
printing_one_string:.asciz "Factorial result of %d: %d\n"
printing_two_string:.asciz "Product result of %d: %d\n"
integer_number: .int 0
option_number: .int 0
.text
.global main
.extern printf

calculateFunc:
	push	{ip, lr}
    MOV R2, #1
    LDR R3, =integer_number
    LDR R3, [R3]
	LOOP:
        CMP R3, #1              @compare if R3 == 1
        BLE END                 @;end loop if R3 == 1
        MUL R2, R3              @; multiply next value
        SUB R3, R4              @;subtract by value of r4, 1 if factorial, 2 if product
        B LOOP                  @;loop
    END:
	pop	{ip, pc}

option1:
    push	{ip, lr}
    BL calculateFunc                @;calculate factorial result
    LDR R1, =integer_number         @;save address of result in R1
    LDR R1, [R1]                    @;get value of result into R1
    LDR R0, =printing_one_string    @;printing format
    BL printf                       @;print result
    pop	{ip, pc}
    
option2:
    push	{ip, lr}
    BL calculateFunc                @;calculate product result
    LDR R1, =integer_number         @;save address of result in R1
    LDR R1, [R1]                    @;get value of result into R1
    LDR R0, =printing_two_string    @;printing format
    BL printf                       @;print result
    POP {ip, pc}

askInteger:
    PUSH {ip, lr}
    LDR R0,=input_integer_string
    BL printf               @;ask for integer number;
    LDR R0, =input_format   @;to specify the input format
    LDR R1, =integer_number @;take in integer number, store in integer_number;
    BL scanf                @;take in input of integer
    POP {ip, pc}

askOption:
    PUSH {ip, lr}
    LDR R0,=input_option_string     @;ask for option;
    BL printf
    LDR R0, =input_format           @;to specify the input format
    LDR R1, =option_number          @;take in option, store in option_number;
    BL scanf                        @;takes in option number
    LDR R4, =option_number
    LDR R4, [R4]                    @;stores value of option in R4
    CMP R4, #1
    CMPNE R4, #2
    BLNE askOption                  @;if option != 1 or 2, branch back and ask again
    POPEQ {ip, pc}

main:
    PUSH {ip, lr}
    BL askInteger
    BL askOption
    CMP R4, #1
    BLEQ option1
    CMP R4, #2
    BLEQ option2
    POP {ip, pc}
