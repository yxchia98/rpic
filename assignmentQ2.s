.data
.balign 4
input_one_string:.asciz "\nEnter an integer number:"
input_two_string:.asciz "\nEnter the Option 1 or 2:"
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
        CMP R3, #1
        BLE END
        MUL R2, R3
        SUB R3, R4
        B LOOP
    END:
	pop	{ip, pc}

option1:
    push	{ip, lr}
    BL calculateFunc
    LDR R1, =integer_number
    LDR R1, [R1]
    LDR R0, =printing_one_string
    BL printf
    pop	{ip, pc}
option2:
    push	{ip, lr}
    BL calculateFunc
    LDR R1, =integer_number
    LDR R1, [R1]
    LDR R0, =printing_two_string
    BL printf
    POP {ip, pc}
askNumber:
    PUSH {ip, lr}
    LDR R0,=input_one_string
    BL printf   @;ask for integer number;
    LDR R0, =input_format   @;take in integer number, store in integer_number;
    LDR R1, =integer_number
    BL scanf
    POP {ip, pc}
askOption:
    PUSH {ip, lr}
    LDR R0,=input_two_string    @;ask for option;
    BL printf
    LDR R0, =input_format   @;take in option, store in option_number;
    LDR R1, =option_number
    BL scanf
    LDR R4, =option_number
    LDR R4, [R4]
    CMP R4, #1
    CMPNE R4, #2
    BLNE askOption
    POPEQ {ip, pc}
main:
    PUSH {ip, lr}
    BL askNumber
    BL askOption
    CMP R4, #1
    BLEQ option1
    CMP R4, #2
    BLEQ option2
    POP {ip, pc}
