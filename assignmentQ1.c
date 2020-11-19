#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int binSub(int operand1, int operand2);
int binAdd(int operand1, int operand2);
int calCarry(int operand1, int operand2);
int binAdd(int operand1, int operand2);
int binSub(int operand1, int operand2);
int binDiv(int operand1, int operand2);
int binDivLoop(int operand1, int operand2);

int isPositive(int n);

int main()
{
    int operand1, operand2, result, remainder;

    printf("Enter Dividend:");                      //get dividend
    scanf("%d", &operand1);                         
    printf("Enter Divisor:");                       //get divisor
    scanf("%d", &operand2);                              //pointer 
    result = binDiv(operand1, operand2);       //pass values into binDiv()
    printf("Binary division for %d(dividend) and %d(divisor) is: %d\n", operand1, operand2, result);   
    return 0;
}

int calCarry(int operand1, int operand2)
{
    int carry;
    carry = (operand1 & operand2) << 1;
    return carry;
}

int binAdd(int operand1, int operand2)
{
    int temp;
    while(operand2!=0)
    {
        temp = calCarry(operand1, operand2);
        operand1 = operand1 ^ operand2;
        operand2 = temp;
    }
    return operand1;
}

int binSub(int operand1, int operand2)                          //Perform binary subtraction using twos complement negation
{
    operand2=binAdd(~operand2,1);                               
    int result=binAdd(operand1,operand2);
    return result;
}

int binDiv(int operand1, int operand2)
{
    int operand1Type, operand2Type, result;
    operand1Type = isPositive(operand1);                        //check for positive or negative value, if positive return 1, if negative return 0
    operand2Type = isPositive(operand2);   

    if(operand1Type == 1 && operand2Type == 1)                  //both operands positive, dont need conversion
    {
        result = binDivLoop(operand1, operand2);
    }
    else if(operand1Type == 1 && operand2Type == 0)             //divisor negative, convert to positive before division
    {
        operand2 = binAdd(~operand2, 1);                        //convert divisor to positive equivalent 
        result = binAdd(~binDivLoop(operand1, operand2), 1);
    }
    else if(operand1Type == 0 && operand2Type == 1)             //dividend negative, convert to positive equivalent before division
    {
        operand1 = binAdd(~operand1, 1);                        //convert dividend to positive equivalent      
        result = binAdd(~binDivLoop(operand1, operand2), 1);
    }
    else                                                        //both operands negative, convert both to positive before division.
    {
        operand1 = binAdd(~operand1, 1);                        //convert both dividend and divisor to positive equivalentb before division
        operand2 = binAdd(~operand2, 1);
        result = binDivLoop(operand1, operand2);
    }
    
    return result;
}

int binDivLoop(int operand1, int operand2)
{
    int i, remainder, divisor, quotient;
    i = 0;
    remainder = operand1;                               //32bit dividend
    divisor = operand2 << 16;                           //extend divisor into 32-bit from 16-bit
    quotient = 0;                                       //initialize quotient

    while(i < 17 && divisor != 0)                       //exits loop after 17 iterations, or when divisor reaches 0
    {
        remainder = binSub(remainder, divisor);         //subtract divisor from remainder, and add back into remainder variable
        if(remainder < 0)                               //if remainder is negative, reinstate previous remainder value
        {
            remainder = binAdd(remainder, divisor);     //add back divisor to get previous value
            quotient = quotient << 1;                   //left shift quotient by 1, clear LSB
        }
        else                                            //when remainder >= 0, left shift quotient and set to 1
        {
            quotient = quotient << 1;                   //left shift quotient
            quotient = quotient | 1;                    //set LSB
        }
        divisor = divisor >> 1;                         //end of every iteration, right shift divisor
        i = binAdd(i, 1);                               //increment counter
    }
    return quotient;
}

int isPositive(int n)
{
    if(((n >> 31) & 0x1) == 0)                          //left shift n by 31 bits, so that we can compare the original MSB with 0x1
    {                                                   //to see if it is negative or positive
        return 1;                                       //return 1 if positive, return 0 if negative
    }
    else
    {
        return 0;
    }
    
}