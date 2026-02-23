#ifndef MATH_EXPRESSION_PARSER_HPP
#define MATH_EXPRESSION_PARSER_HPP

#include <QString>
#include <QMap>
#include <QSharedPointer>
#include <cmath>

/**
 * @brief The MathExpressionParser class parses and evaluates mathematical expressions
 *
 * Supports:
 * - Arithmetic operators: +, -, *, /, ^ (power)
 * - Functions: abs(), sqrt(), log(), exp(), sin(), cos(), tan()
 * - Constants: pi, e
 * - Parentheses for precedence
 * - Variable substitution
 *
 * TODO: Future enhancements:
 * - Conditional operations (if/then/else)
 * - Aggregations (moving_avg, min, max over windows)
 * - Derivatives and integrals
 */
class MathExpressionParser
{
public:
    MathExpressionParser();
    ~MathExpressionParser();

    /**
     * @brief Parse an expression string into an internal representation
     * @param expression The mathematical expression (e.g., "a * b + c")
     * @return true if parsing succeeded, false otherwise
     */
    bool parse(const QString& expression);

    /**
     * @brief Evaluate the parsed expression with given variable values
     * @param variables Map of variable names to their values
     * @param result Output parameter for the computed result
     * @return true if evaluation succeeded, false otherwise
     */
    bool evaluate(const QMap<QString, double>& variables, double& result) const;

    /**
     * @brief Get the last error message
     * @return Error message string, or empty if no error
     */
    QString getError() const { return errorMessage; }

    /**
     * @brief Get list of variables used in the expression
     * @return List of variable names
     */
    QStringList getVariables() const;

private:
    // Internal expression node types
    enum NodeType
    {
        NODE_NUMBER,
        NODE_VARIABLE,
        NODE_OPERATOR,
        NODE_FUNCTION
    };

    enum OperatorType
    {
        OP_ADD,
        OP_SUBTRACT,
        OP_MULTIPLY,
        OP_DIVIDE,
        OP_POWER,
        OP_NEGATE  // Unary minus
    };

    enum FunctionType
    {
        FUNC_ABS,
        FUNC_SQRT,
        FUNC_LOG,
        FUNC_EXP,
        FUNC_SIN,
        FUNC_COS,
        FUNC_TAN
    };

    // Expression tree node
    struct ExpressionNode
    {
        NodeType type;

        // Value for NUMBER nodes
        double numberValue = 0.0;

        // Variable name for VARIABLE nodes
        QString variableName;

        // Operator type for OPERATOR nodes
        OperatorType operatorType;

        // Function type for FUNCTION nodes
        FunctionType functionType;

        // Child nodes
        QSharedPointer<ExpressionNode> left;
        QSharedPointer<ExpressionNode> right;
        QSharedPointer<ExpressionNode> argument;  // For functions
    };

    typedef QSharedPointer<ExpressionNode> NodePtr;

    // Tokenizer
    struct Token
    {
        enum Type
        {
            TOKEN_NUMBER,
            TOKEN_VARIABLE,
            TOKEN_OPERATOR,
            TOKEN_FUNCTION,
            TOKEN_LEFT_PAREN,
            TOKEN_RIGHT_PAREN,
            TOKEN_COMMA,
            TOKEN_END
        };

        Type type;
        QString value;
        double numberValue = 0.0;
    };

    // Parsing methods
    QList<Token> tokenize(const QString& expression);
    NodePtr parseExpression(const QList<Token>& tokens, int& pos);
    NodePtr parseTerm(const QList<Token>& tokens, int& pos);
    NodePtr parseFactor(const QList<Token>& tokens, int& pos);
    NodePtr parsePower(const QList<Token>& tokens, int& pos);
    NodePtr parseUnary(const QList<Token>& tokens, int& pos);
    NodePtr parsePrimary(const QList<Token>& tokens, int& pos);

    // Evaluation method
    bool evaluateNode(const NodePtr& node, const QMap<QString, double>& variables, double& result) const;

    // Helper methods
    void collectVariables(const NodePtr& node, QStringList& variables) const;
    bool isOperator(QChar c) const;

    // Parsed expression tree
    NodePtr rootNode;

    // Error handling
    QString errorMessage;
};

#endif // MATH_EXPRESSION_PARSER_HPP
