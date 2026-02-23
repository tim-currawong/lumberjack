#include "math_expression_parser.hpp"
#include <QRegularExpression>
#include <QDebug>

MathExpressionParser::MathExpressionParser()
{
}

MathExpressionParser::~MathExpressionParser()
{
}

/**
 * @brief Parse a mathematical expression into an internal tree structure
 *
 * This is a two-phase process:
 * 1. Tokenization: Convert string into tokens (numbers, operators, functions, etc.)
 * 2. Parsing: Build expression tree using recursive descent parser
 *
 * The parser handles operator precedence through function hierarchy:
 * - parseExpression: +, - (lowest precedence)
 * - parseTerm: *, /
 * - parsePower: ^ (right-associative)
 * - parseUnary: unary -
 * - parsePrimary: numbers, variables, functions, parentheses
 *
 * @param expression The mathematical expression string (e.g., "a * b + c")
 * @return true if parsing succeeded, false otherwise (check getError())
 */
bool MathExpressionParser::parse(const QString& expression)
{
    errorMessage.clear();
    rootNode.reset();

    if (expression.trimmed().isEmpty())
    {
        errorMessage = "Empty expression";
        return false;
    }

    // Phase 1: Tokenize the expression into meaningful units
    QList<Token> tokens = tokenize(expression);

    if (!errorMessage.isEmpty())
    {
        return false;
    }

    // Phase 2: Parse tokens into expression tree using recursive descent
    int pos = 0;
    try
    {
        rootNode = parseExpression(tokens, pos);

        // Ensure we consumed all tokens (except the END marker)
        if (pos < tokens.size() && tokens[pos].type != Token::TOKEN_END)
        {
            errorMessage = QString("Unexpected token at position %1: %2").arg(pos).arg(tokens[pos].value);
            return false;
        }
    }
    catch (const QString& error)
    {
        errorMessage = error;
        return false;
    }

    return rootNode != nullptr;
}

/**
 * @brief Tokenize a mathematical expression string
 *
 * Converts the input string into a list of tokens for parsing.
 * Handles:
 * - Numbers (integers and decimals like 3.14)
 * - Variables (alphanumeric + underscore, e.g., "rpm1", "temp_sensor")
 * - Functions (abs, sqrt, log, exp, sin, cos, tan)
 * - Constants (pi, e)
 * - Operators (+, -, *, /, ^)
 * - Parentheses and commas
 *
 * @param expression The input expression string
 * @return List of tokens, or empty list on error (check errorMessage)
 */
QList<MathExpressionParser::Token> MathExpressionParser::tokenize(const QString& expression)
{
    QList<Token> tokens;
    QString expr = expression.trimmed();
    int i = 0;

    while (i < expr.length())
    {
        QChar c = expr[i];

        // Skip whitespace
        if (c.isSpace())
        {
            i++;
            continue;
        }

        // Numbers (including decimals)
        if (c.isDigit() || c == '.')
        {
            int start = i;
            while (i < expr.length() && (expr[i].isDigit() || expr[i] == '.'))
            {
                i++;
            }

            Token token;
            token.type = Token::TOKEN_NUMBER;
            token.value = expr.mid(start, i - start);
            token.numberValue = token.value.toDouble();
            tokens.append(token);
            continue;
        }

        // Variables and functions (start with letter)
        if (c.isLetter())
        {
            int start = i;
            while (i < expr.length() && (expr[i].isLetterOrNumber() || expr[i] == '_'))
            {
                i++;
            }

            Token token;
            token.value = expr.mid(start, i - start);

            // Check if it's a known function
            if (token.value == "abs" || token.value == "sqrt" || token.value == "log" ||
                token.value == "exp" || token.value == "sin" || token.value == "cos" ||
                token.value == "tan")
            {
                token.type = Token::TOKEN_FUNCTION;
            }
            // Check if it's a constant
            else if (token.value == "pi")
            {
                token.type = Token::TOKEN_NUMBER;
                token.numberValue = M_PI;
            }
            else if (token.value == "e")
            {
                token.type = Token::TOKEN_NUMBER;
                token.numberValue = M_E;
            }
            else
            {
                token.type = Token::TOKEN_VARIABLE;
            }

            tokens.append(token);
            continue;
        }

        // Operators
        if (isOperator(c))
        {
            Token token;
            token.type = Token::TOKEN_OPERATOR;
            token.value = c;
            tokens.append(token);
            i++;
            continue;
        }

        // Parentheses
        if (c == '(')
        {
            Token token;
            token.type = Token::TOKEN_LEFT_PAREN;
            token.value = c;
            tokens.append(token);
            i++;
            continue;
        }

        if (c == ')')
        {
            Token token;
            token.type = Token::TOKEN_RIGHT_PAREN;
            token.value = c;
            tokens.append(token);
            i++;
            continue;
        }

        // Comma (for multi-arg functions in the future)
        if (c == ',')
        {
            Token token;
            token.type = Token::TOKEN_COMMA;
            token.value = c;
            tokens.append(token);
            i++;
            continue;
        }

        errorMessage = QString("Invalid character at position %1: %2").arg(i).arg(c);
        return QList<Token>();
    }

    // Add end token marker to simplify parsing logic
    Token endToken;
    endToken.type = Token::TOKEN_END;
    tokens.append(endToken);

    return tokens;
}

/**
 * @brief Parse an expression (lowest precedence: addition and subtraction)
 *
 * Grammar: expression = term (('+' | '-') term)*
 *
 * This handles left-associative binary operators with lowest precedence.
 * For example: "a + b - c" is parsed as "(a + b) - c"
 *
 * @param tokens List of tokens to parse
 * @param pos Current position in token list (updated as tokens are consumed)
 * @return Expression tree node representing the parsed expression
 */
MathExpressionParser::NodePtr MathExpressionParser::parseExpression(const QList<Token>& tokens, int& pos)
{
    NodePtr left = parseTerm(tokens, pos);

    while (pos < tokens.size() &&
           tokens[pos].type == Token::TOKEN_OPERATOR &&
           (tokens[pos].value == "+" || tokens[pos].value == "-"))
    {
        QString op = tokens[pos].value;
        pos++;

        NodePtr right = parseTerm(tokens, pos);

        NodePtr node = NodePtr::create();
        node->type = NODE_OPERATOR;
        node->operatorType = (op == "+") ? OP_ADD : OP_SUBTRACT;
        node->left = left;
        node->right = right;

        left = node;
    }

    return left;
}

/**
 * @brief Parse a term (medium precedence: multiplication and division)
 *
 * Grammar: term = power (('*' | '/') power)*
 *
 * Handles left-associative operators with higher precedence than +/-.
 * For example: "a * b / c" is parsed as "(a * b) / c"
 *
 * @param tokens List of tokens to parse
 * @param pos Current position in token list (updated as tokens are consumed)
 * @return Expression tree node representing the parsed term
 */
MathExpressionParser::NodePtr MathExpressionParser::parseTerm(const QList<Token>& tokens, int& pos)
{
    NodePtr left = parsePower(tokens, pos);

    while (pos < tokens.size() &&
           tokens[pos].type == Token::TOKEN_OPERATOR &&
           (tokens[pos].value == "*" || tokens[pos].value == "/"))
    {
        QString op = tokens[pos].value;
        pos++;

        NodePtr right = parsePower(tokens, pos);

        NodePtr node = NodePtr::create();
        node->type = NODE_OPERATOR;
        node->operatorType = (op == "*") ? OP_MULTIPLY : OP_DIVIDE;
        node->left = left;
        node->right = right;

        left = node;
    }

    return left;
}

/**
 * @brief Parse power operation (highest precedence: exponentiation)
 *
 * Grammar: power = unary ('^' power)?
 *
 * Handles RIGHT-associative exponentiation operator.
 * For example: "2 ^ 3 ^ 2" is parsed as "2 ^ (3 ^ 2)" = 2^9 = 512
 * (not "(2 ^ 3) ^ 2" = 8^2 = 64)
 *
 * @param tokens List of tokens to parse
 * @param pos Current position in token list (updated as tokens are consumed)
 * @return Expression tree node representing the power operation
 */
MathExpressionParser::NodePtr MathExpressionParser::parsePower(const QList<Token>& tokens, int& pos)
{
    NodePtr left = parseUnary(tokens, pos);

    if (pos < tokens.size() &&
        tokens[pos].type == Token::TOKEN_OPERATOR &&
        tokens[pos].value == "^")
    {
        pos++;
        NodePtr right = parsePower(tokens, pos);  // Right-associative

        NodePtr node = NodePtr::create();
        node->type = NODE_OPERATOR;
        node->operatorType = OP_POWER;
        node->left = left;
        node->right = right;

        return node;
    }

    return left;
}

// Parse unary: handles unary minus
MathExpressionParser::NodePtr MathExpressionParser::parseUnary(const QList<Token>& tokens, int& pos)
{
    if (pos < tokens.size() &&
        tokens[pos].type == Token::TOKEN_OPERATOR &&
        tokens[pos].value == "-")
    {
        pos++;
        NodePtr operand = parseUnary(tokens, pos);

        NodePtr node = NodePtr::create();
        node->type = NODE_OPERATOR;
        node->operatorType = OP_NEGATE;
        node->left = operand;

        return node;
    }

    return parsePrimary(tokens, pos);
}

// Parse primary: handles numbers, variables, functions, and parentheses
MathExpressionParser::NodePtr MathExpressionParser::parsePrimary(const QList<Token>& tokens, int& pos)
{
    if (pos >= tokens.size())
    {
        throw QString("Unexpected end of expression");
    }

    Token token = tokens[pos];

    // Number
    if (token.type == Token::TOKEN_NUMBER)
    {
        pos++;
        NodePtr node = NodePtr::create();
        node->type = NODE_NUMBER;
        node->numberValue = token.numberValue;
        return node;
    }

    // Variable
    if (token.type == Token::TOKEN_VARIABLE)
    {
        pos++;
        NodePtr node = NodePtr::create();
        node->type = NODE_VARIABLE;
        node->variableName = token.value;
        return node;
    }

    // Function
    if (token.type == Token::TOKEN_FUNCTION)
    {
        QString funcName = token.value;
        pos++;

        // Expect '('
        if (pos >= tokens.size() || tokens[pos].type != Token::TOKEN_LEFT_PAREN)
        {
            throw QString("Expected '(' after function name");
        }
        pos++;

        // Parse argument
        NodePtr argument = parseExpression(tokens, pos);

        // Expect ')'
        if (pos >= tokens.size() || tokens[pos].type != Token::TOKEN_RIGHT_PAREN)
        {
            throw QString("Expected ')' after function argument");
        }
        pos++;

        NodePtr node = NodePtr::create();
        node->type = NODE_FUNCTION;
        node->argument = argument;

        // Set function type
        if (funcName == "abs") node->functionType = FUNC_ABS;
        else if (funcName == "sqrt") node->functionType = FUNC_SQRT;
        else if (funcName == "log") node->functionType = FUNC_LOG;
        else if (funcName == "exp") node->functionType = FUNC_EXP;
        else if (funcName == "sin") node->functionType = FUNC_SIN;
        else if (funcName == "cos") node->functionType = FUNC_COS;
        else if (funcName == "tan") node->functionType = FUNC_TAN;
        else
        {
            throw QString("Unknown function: %1").arg(funcName);
        }

        return node;
    }

    // Parenthesized expression
    if (token.type == Token::TOKEN_LEFT_PAREN)
    {
        pos++;
        NodePtr node = parseExpression(tokens, pos);

        if (pos >= tokens.size() || tokens[pos].type != Token::TOKEN_RIGHT_PAREN)
        {
            throw QString("Expected ')'");
        }
        pos++;

        return node;
    }

    throw QString("Unexpected token: %1").arg(token.value);
}

bool MathExpressionParser::evaluate(const QMap<QString, double>& variables, double& result) const
{
    if (!rootNode)
    {
        return false;
    }

    return evaluateNode(rootNode, variables, result);
}

bool MathExpressionParser::evaluateNode(const NodePtr& node, const QMap<QString, double>& variables, double& result) const
{
    if (!node)
    {
        return false;
    }

    switch (node->type)
    {
    case NODE_NUMBER:
        result = node->numberValue;
        return true;

    case NODE_VARIABLE:
        if (!variables.contains(node->variableName))
        {
            return false;
        }
        result = variables[node->variableName];
        return true;

    case NODE_OPERATOR:
    {
        double leftVal = 0.0;
        double rightVal = 0.0;

        if (node->operatorType == OP_NEGATE)
        {
            if (!evaluateNode(node->left, variables, leftVal))
            {
                return false;
            }
            result = -leftVal;
            return true;
        }

        if (!evaluateNode(node->left, variables, leftVal) ||
            !evaluateNode(node->right, variables, rightVal))
        {
            return false;
        }

        switch (node->operatorType)
        {
        case OP_ADD:
            result = leftVal + rightVal;
            return true;

        case OP_SUBTRACT:
            result = leftVal - rightVal;
            return true;

        case OP_MULTIPLY:
            result = leftVal * rightVal;
            return true;

        case OP_DIVIDE:
            if (rightVal == 0.0)
            {
                return false;  // Division by zero
            }
            result = leftVal / rightVal;
            return true;

        case OP_POWER:
            result = pow(leftVal, rightVal);
            return true;

        default:
            return false;
        }
    }

    case NODE_FUNCTION:
    {
        double argVal = 0.0;
        if (!evaluateNode(node->argument, variables, argVal))
        {
            return false;
        }

        switch (node->functionType)
        {
        case FUNC_ABS:
            result = fabs(argVal);
            return true;

        case FUNC_SQRT:
            if (argVal < 0.0)
            {
                return false;  // Negative sqrt
            }
            result = sqrt(argVal);
            return true;

        case FUNC_LOG:
            if (argVal <= 0.0)
            {
                return false;  // Log of non-positive
            }
            result = log(argVal);
            return true;

        case FUNC_EXP:
            result = exp(argVal);
            return true;

        case FUNC_SIN:
            result = sin(argVal);
            return true;

        case FUNC_COS:
            result = cos(argVal);
            return true;

        case FUNC_TAN:
            result = tan(argVal);
            return true;

        default:
            return false;
        }
    }

    default:
        return false;
    }
}

QStringList MathExpressionParser::getVariables() const
{
    QStringList variables;
    if (rootNode)
    {
        collectVariables(rootNode, variables);
    }
    return variables;
}

void MathExpressionParser::collectVariables(const NodePtr& node, QStringList& variables) const
{
    if (!node)
    {
        return;
    }

    if (node->type == NODE_VARIABLE)
    {
        if (!variables.contains(node->variableName))
        {
            variables.append(node->variableName);
        }
    }

    collectVariables(node->left, variables);
    collectVariables(node->right, variables);
    collectVariables(node->argument, variables);
}

bool MathExpressionParser::isOperator(QChar c) const
{
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '^';
}
