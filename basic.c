#include "basic.h"
#include "io.h"

#define True 1
#define False 0
// define the maximum int to be not a number(int)
#define NaN 0x7FFFFFFF
// define the minimum int to be error
#define ERROR 0x80000000
// not a line
#define NaL 0xFFFF

#define LINEBUFSIZE 64
#define PROGBUFSIZE 1024
#define INDEXBUFSIZE 64
#define STACKSIZE 32

// additional char for \0 in the end
char lineBuf[LINEBUFSIZE + 1];
int lineBufCnt = 0;
char progBuf[PROGBUFSIZE + 1];
int progBufCnt = 0;
// line number | offset for each line
unsigned short indexBuf[INDEXBUFSIZE * 2];
int indexBufCnt = 0;
// line number stack, for loop and gotosub
unsigned short lineNumStack[STACKSIZE];
int lineNumSP = 0;
// variables A-Z
int variables[26];

int getNextOffset(char *line, int offset)
{
    while (line[offset] != '\0' && line[offset] != ' ')
    {
        offset = offset + 1;
    }
    // return the position of next valid char or a value larger than the size
    return offset + 1;
}

// return the updated offset of line
int matchCMD(const char *cmd, char *line, int offset)
{
    int i = 0;
    int j = offset;
    while (cmd[i] != '\0')
    {
        // taking into account of end of line, since cmd[i] is not \0
        if (cmd[i] != line[j])
        {
            return False;
        }
        i = i + 1;
        j = j + 1;
    }
    if (line[j] == '\0' || line[j] == ' ')
    {
        // return the position of next valid char or a value larger than the size
        return j + 1;
    }
    return False;
}

int getInt2(char *numStr, int len)
{
    int i = 0;
    int result = 0;
    int negative = False;
    int c = numStr[i];
    if (c == '-')
    {
        negative = True;
        i = i + 1;
    }
    while (i < len)
    {
        c = numStr[i];
        if (c >= '0' && c <= '9')
        {
            result = result * 10 + (c - '0');
            i = i + 1;
        }
        else
        {
            return NaN;
        }
    }
    if (i == 0)
    {
        return NaN;
    }
    if (negative)
    {
        if (i == 1)
        {
            return NaN;
        }
        result = result * -1;
    }
    return result;
}

// return the int value or NaN, need to calculate offset separately
int getInt(char *line, int offset)
{
    int len = getNextOffset(line, offset) - offset - 1;
    return getInt2(line + offset, len);
}

void putStr(const char *s)
{
    int i = 0;
    while (s[i] != '\0')
    {
        putChar(s[i]);
        i = i + 1;
    }
    return;
}

void putInt(int num)
{
    if (num == NaN)
    {
        putStr("NaN");
        return;
    }
    if (num == 0)
    {
        putChar('0');
        return;
    }
    if (num < 0)
    {
        putChar('-');
        num = num * -1;
    }
    int i = 0;
    char bits[10] = {0};
    while (num != 0)
    {
        bits[i] = (num % 10) + '0';
        num = num / 10;
        i = i + 1;
    }
    while (i != 0)
    {
        i = i - 1;
        putChar(bits[i]);
    }
    return;
}

// stack management
int pushStack(int lineNum)
{
    if (lineNumSP < STACKSIZE)
    {
        lineNumStack[lineNumSP] = lineNum;
        lineNumSP = lineNumSP + 1;
        return 0;
    }
    else
    {
        putStr("STACK OVERFLOW");
        return 1;
    }
}

int popStack(void)
{
    if (lineNumSP > 0)
    {
        lineNumSP = lineNumSP - 1;
        return lineNumStack[lineNumSP];
    }
    else
    {
        return NaN;
    }
}

// return line index or (NaL + where should this line be inserted)
int findLineIndex(int lineNum)
{
    int l = 0;
    int r = indexBufCnt / 2 - 1;
    int m;
    int cLineNum;
    while (r >= l)
    {
        m = (r + l) / 2;
        cLineNum = indexBuf[m * 2];
        if (cLineNum > lineNum)
        {
            r = m - 1;
        }
        else if (cLineNum < lineNum)
        {
            l = m + 1;
        }
        else
        {
            return m * 2;
        }
    }
    return l * 2 + NaL;
}

int insertNewLine(int lineNum)
{
    int lineIndex = findLineIndex(lineNum);
    if (lineIndex >= NaL)
    {
        // not found, need to add new line
        if (indexBufCnt >= INDEXBUFSIZE * 2)
        {
            putStr("PROG OVERFLOW");
            return 1;
        }
        lineIndex = lineIndex - NaL;
        if (lineIndex != indexBufCnt)
        {
            // need to move index buf back from lineIndex
            int i = indexBufCnt + 1;
            while (i >= lineIndex + 2)
            {
                indexBuf[i] = indexBuf[i - 2];
                i = i - 1;
            }
        }
        // else, add at the end
        indexBufCnt = indexBufCnt + 2;
    }
    indexBuf[lineIndex] = lineNum;
    indexBuf[lineIndex + 1] = progBufCnt;
    int i = getNextOffset(lineBuf, 0);
    while (i < lineBufCnt)
    {
        if (progBufCnt < PROGBUFSIZE)
        {
            progBuf[progBufCnt] = lineBuf[i];
            progBufCnt = progBufCnt + 1;
            i = i + 1;
        }
        else
        {
            putStr("PROG OVERFLOW");
            return 1;
        }
    }
    return 0;
}

void putERROR(int lineNum)
{
    putStr("ERROR IN LINE: ");
    putInt(lineNum);
    putStr("\r\n");
}

void putSyntaxERROR()
{
    putStr("SYNTAX ERROR\r\n");
}

// expression commands
const char cmdPRINT[] = "PRINT";
const char cmdGOTO[] = "GOTO";
const char cmdEND[] = "END";
const char cmdIF[] = "IF";
const char cmdTHEN[] = "THEN";

// TODO
const char cmdWHILE[] = "WHILE";
const char cmdWEND[] = "WEND";
const char cmdGOTOSUB[] = "GOTOSUB";
const char cmdRETURN[] = "RETURN";

// commands
const char cmdLIST[] = "LIST";
const char cmdRUN[] = "RUN";
const char cmdNEW[] = "NEW";

// poiority: low to high
// =  ||  &&  ==  !=  <  <=  >  >=  +  -  *  /  ()
enum operatorPoriorities
{
    pAssign,
    pLogicOr,
    pLogicAnd,
    pEqual,
    pUnequal = pEqual,
    pSmaller,
    pSmallerOrEqual = pSmaller,
    pGreater = pSmaller,
    pGreaterOrEqual = pSmaller,
    pAdd,
    pSubract = pAdd,
    pMultiply,
    pDevide = pMultiply,
    pBracket
};

enum operators
{
    opAssign,
    opLogicOr,
    opLogicAnd,
    opEqual,
    opUnequal,
    opSmaller,
    opSmallerOrEqual,
    opGreater,
    opGreaterOrEqual,
    opAdd,
    opSubract,
    opMultiply,
    opDevide,
    opBracket
};

// input expression is the pointer to the expression,
// no input offset is taken
int evalExpression2(char *expression, int len)
{
    // find next operator
    int bracketCnt = 0;
    int i = 0;
    int lowestPriority = NaN;
    int operator;
    int lowPPosition;
    int c;
    while (i < len)
    {
        c = expression[i];
        if (c == '(')
        {
            if (lowestPriority > pBracket)
            {
                lowestPriority = pBracket;
                operator= opBracket;
                lowPPosition = i;
            }
            bracketCnt = bracketCnt + 1;
            i = i + 1;
            continue;
        }
        if (c == ')')
        {
            bracketCnt = bracketCnt - 1;
            i = i + 1;
            continue;
        }
        if (bracketCnt != 0)
        {
            i = i + 1;
            continue;
        }
        // other operators
        if (c == '=')
        {
            if (expression[i + 1] == '=')
            {
                if (lowestPriority > pEqual)
                {
                    lowestPriority = pEqual;
                    operator= opEqual;
                    lowPPosition = i;
                }
                i = i + 1;
            }
            else if (lowestPriority > pAssign)
            {
                lowestPriority = pAssign;
                operator= opAssign;
                lowPPosition = i;
            }
        }
        else if (c == '|')
        {
            if (expression[i + 1] == '|')
            {
                if (lowestPriority > pLogicOr)
                {
                    lowestPriority = pLogicOr;
                    operator= opLogicOr;
                    lowPPosition = i;
                }
                i = i + 1;
            }
            else
            {
                putSyntaxERROR();
                return NaN;
            }
        }
        else if (c == '&')
        {
            if (expression[i + 1] == '&')
            {
                if (lowestPriority > pLogicAnd)
                {
                    lowestPriority = pLogicAnd;
                    operator= opLogicAnd;
                    lowPPosition = i;
                }
                i = i + 1;
            }
            else
            {
                putSyntaxERROR();
                return NaN;
            }
        }
        else if (c == '!')
        {
            if (expression[i + 1] == '=')
            {
                if (lowestPriority > pUnequal)
                {
                    lowestPriority = pUnequal;
                    operator= opUnequal;
                    lowPPosition = i;
                }
                i = i + 1;
            }
            else
            {
                putSyntaxERROR();
                return NaN;
            }
        }
        else if (c == '<')
        {
            if (expression[i + 1] == '=')
            {
                if (lowestPriority > pSmallerOrEqual)
                {
                    lowestPriority = pSmallerOrEqual;
                    operator= opSmallerOrEqual;
                    lowPPosition = i;
                }
                i = i + 1;
            }
            else if (lowestPriority > pSmaller)
            {
                lowestPriority = pSmaller;
                operator= opSmaller;
                lowPPosition = i;
            }
        }
        else if (c == '>')
        {
            if (expression[i + 1] == '=')
            {
                if (lowestPriority > pGreaterOrEqual)
                {
                    lowestPriority = pGreaterOrEqual;
                    operator= opGreaterOrEqual;
                    lowPPosition = i;
                }
                i = i + 1;
            }
            else if (lowestPriority > pGreater)
            {
                lowestPriority = pGreater;
                operator= opGreater;
                lowPPosition = i;
            }
        }
        else if (c == '+')
        {
            if (lowestPriority > pAdd)
            {
                lowestPriority = pAdd;
                operator= opAdd;
                lowPPosition = i;
            }
        }
        else if (c == '-')
        {
            if (lowestPriority > pSubract)
            {
                lowestPriority = pSubract;
                operator= opSubract;
                lowPPosition = i;
            }
        }
        else if (c == '*')
        {
            if (lowestPriority > pMultiply)
            {
                lowestPriority = pMultiply;
                operator= opMultiply;
                lowPPosition = i;
            }
        }
        else if (c == '/')
        {
            if (lowestPriority > pDevide)
            {
                lowestPriority = pDevide;
                operator= opDevide;
                lowPPosition = i;
            }
        }
        i = i + 1;
    }

    if (lowestPriority != NaN)
    {
        // handle operators
        int v1;
        int v2;
        // get values
        switch (operator)
        {
        case opLogicOr:
        case opLogicAnd:
        case opEqual:
        case opUnequal:
        case opSmallerOrEqual:
        case opGreaterOrEqual:
            v1 = evalExpression2(expression, lowPPosition);
            v2 = evalExpression2(expression + lowPPosition + 2, len - lowPPosition - 2);
            if (v1 == NaN || v2 == NaN)
            {
                return NaN;
            }
            break;
        case opSmaller:
        case opGreater:
        case opAdd:
        case opSubract:
        case opMultiply:
        case opDevide:
            v1 = evalExpression2(expression, lowPPosition);
            v2 = evalExpression2(expression + lowPPosition + 1, len - lowPPosition - 1);
            if (v1 == NaN || v2 == NaN)
            {
                return NaN;
            }
            break;
        default:
            break;
        }
        // calculate
        switch (operator)
        {
        case opAssign:
            c = expression[0];
            if (c >= 'A' && c <= 'Z' && expression[1] == '=')
            {
                variables[c - 'A'] = evalExpression2(expression + 2, len - 2);
            }
            else
            {
                // middle line assigning is not supported
                putSyntaxERROR();
                return NaN;
            }
            // the reulst of an assign is always 1
            return 1;
        case opLogicOr:
            return v1 || v2;
        case opLogicAnd:
            return v1 && v2;
        case opEqual:
            return v1 == v2;
        case opUnequal:
            return v1 != v2;
        case opSmaller:
            return v1 < v2;
        case opSmallerOrEqual:
            return v1 <= v2;
        case opGreater:
            return v1 > v2;
        case opGreaterOrEqual:
            return v1 >= v2;
        case opAdd:
            return v1 + v2;
        case opSubract:
            return v1 - v2;
        case opMultiply:
            return v1 * v2;
        case opDevide:
            if (v2 == 0)
            {
                return NaN;
            }
            return v1 / v2;
        case opBracket:
            return evalExpression2(expression + 1, len - 2);
        default:
            return NaN;
        }
    }
    else
    {
        // no operator found
        if (len == 0)
        {
            return 0;
        }
        c = expression[0];
        if (len == 1 && c >= 'A' && c <= 'Z')
        {
            return variables[c - 'A'];
        }
        else
        {
            return getInt2(expression, len);
        }
    }
}

// ([^\s\0]*)[\s\0]
// apply the calculations and return the result
int evalExpression(char *line, int offset)
{
    int len = getNextOffset(line, offset) - offset - 1;
    return evalExpression2(line + offset, len);
}

int gotoLine(int lineNum)
{
    int index = findLineIndex(lineNum);
    if (index >= NaL)
    {
        putStr("INVALID LINE:");
        putInt(lineNum);
        putStr("\r\n");
        return ERROR;
    }
    return index;
}
// GOTO\s[0-9]*[\s\0]
// return the index to goto or NaN,
int GOTO(char *line, int offset)
{
    // 5 = len('GOTO ')
    if (line[offset + 4] == '\0')
    {
        putSyntaxERROR();
        return ERROR;
    }
    offset = offset + 5;
    int lineNum = getInt(line, offset);
    return gotoLine(lineNum);
}

int PRINT(char *line, int offset)
{
    offset = offset + 6;
    int firstChar;
    while (line[offset - 1] != '\0')
    {
        firstChar = line[offset];
        if (firstChar == '"')
        {
            // print string
            // skip the fist "
            offset = offset + 1;
            while (line[offset] != '"' && line[offset] != '\0')
            {
                putChar(line[offset]);
                offset = offset + 1;
            }
            if (line[offset] == '"')
            {
                // skip the second "
                offset = offset + 1;
            }
            offset = offset + 1;
        }
        else
        {
            // print result of expression
            int result = evalExpression(line, offset);
            putInt(result);
            offset = getNextOffset(line, offset);
        }
    }
    putStr("\r\n");
    return 0;
}

// return line index for then, return NaL for else, return ERROR for error
int IF(char *line, int offset)
{
    // 3 = len('IF ')
    if (line[offset + 2] == '\0')
    {
        putSyntaxERROR();
        return ERROR;
    }
    offset = offset + 3;
    int condition = evalExpression(line, offset);
    if (condition == NaN)
    {
        condition = False;
    }
    if (condition)
    {
        offset = getNextOffset(line, offset);
        if (matchCMD(cmdTHEN, line, offset))
        {
            // len('THEN ') = 5
            if (line[offset + 4] == '\0')
            {
                putSyntaxERROR();
                return ERROR;
            }
            offset = offset + 5;
            int nextLine = getInt(line, offset);
            return gotoLine(nextLine);
        }
        else
        {
            putSyntaxERROR();
            return ERROR;
        }
    }
    else
    {
        return NaL;
    }
}

void RUN(void)
{
    // index counter
    int IC = 0;
    // program counter (line number)
    int PC;
    // offset of program buffer
    int offset;
    while (True)
    {
        // end
        if (IC >= indexBufCnt)
        {
            break;
        }
        PC = indexBuf[IC];
        offset = indexBuf[IC + 1];
        if (matchCMD(cmdEND, progBuf, offset))
        {
            break;
        }
        else if (matchCMD(cmdGOTO, progBuf, offset))
        {
            IC = GOTO(progBuf, offset);
            if (IC == ERROR)
            {
                putERROR(PC);
                break;
            }
            continue;
        }
        else if (matchCMD(cmdPRINT, progBuf, offset))
        {
            if (PRINT(progBuf, offset) == ERROR)
            {
                putERROR(PC);
                break;
            }
        }
        else if (matchCMD(cmdIF, progBuf, offset))
        {
            int nextLineIndex = IF(progBuf, offset);
            if (nextLineIndex == ERROR)
            {
                putERROR(PC);
                break;
            }
            if (nextLineIndex != NaL)
            {
                IC = nextLineIndex;
                continue;
            }
        }
        else
        {
            if (evalExpression(progBuf, offset) == ERROR)
            {
                putERROR(PC);
                break;
            }
        }
        IC = IC + 2;
    }
    return;
}

void LIST(void)
{
    int i = 0;
    while (i < indexBufCnt)
    {
        putInt(indexBuf[i]);
        putChar('\t');
        int j = indexBuf[i + 1];
        putStr(progBuf + j);
        putStr("\r\n");
        i = i + 2;
    }
    return;
}

void NEW(void)
{
    int i = 0;
    while (i < 26)
    {
        variables[i] = 0;
        i = i + 1;
    }
    indexBufCnt = 0;
    lineBufCnt = 0;
    progBufCnt = 0;
    lineNumSP = 0;
    return;
}

int processLine(void)
{
    putStr("\b\b  \r\n");
    lineBuf[lineBufCnt] = '\0';
    lineBufCnt = lineBufCnt + 1;
    int num = getInt(lineBuf, 0);
    if (num != NaN)
    {
        if (insertNewLine(num))
        {
            lineBufCnt = 0;
            return 1;
        }
    }
    else if (matchCMD(cmdLIST, lineBuf, 0))
    {
        LIST();
    }
    else if (matchCMD(cmdRUN, lineBuf, 0))
    {
        RUN();
    }
    else if (matchCMD(cmdNEW, lineBuf, 0))
    {
        NEW();
    }
    lineBufCnt = 0;
    return 0;
}

int processChar(int num)
{
    // esc
    if (num == 27)
    {
        putStr("\b\b  \r");
        return 1;
    }
    if (num == '\r')
    {
        return processLine();
    }
    if (num == 127)
    {
        putStr("\b\b\b   \b\b\b");
        if (lineBufCnt > 0)
        {
            lineBufCnt = lineBufCnt - 1;
        }
        return 0;
    }
    if (num >= 'a' && num <= 'z')
    {
        num = num - ('a' - 'A');
    }
    putStr("\b \b");
    if (lineBufCnt < LINEBUFSIZE)
    {
        putChar(num);
        lineBuf[lineBufCnt] = num;
        lineBufCnt = lineBufCnt + 1;
    }
    return 0;
}

void startBasic(void)
{
    NEW();
    while (1)
    {
        int num = getChar();
        // printf("%d",num);
        if (processChar(num))
        {
            break;
        }
    }
}
