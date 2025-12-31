import ts from "typescript";

import type { DeclaredSymbols } from "../../ast/symbols";
import type { Node } from "../../ast/types";
import { CodeGenerator } from "./";
import { visitClassDeclaration } from "./class-handlers";
import {
  visitCaseClause,
  visitDefaultClause,
  visitDoStatement,
  visitForInStatement,
  visitForOfStatement,
  visitForStatement,
  visitSwitchStatement,
  visitWhileStatement,
} from "./control-flow-handlers";
import {
  visitVariableDeclaration,
  visitVariableDeclarationList,
} from "./declaration-handlers";
import {
  visitArrayLiteralExpression,
  visitAwaitExpression,
  visitBinaryExpression,
  visitCallExpression,
  visitConditionalExpression,
  visitDeleteExpression,
  visitElementAccessExpression,
  visitNewExpression,
  visitObjectLiteralExpression,
  visitParenthesizedExpression,
  visitPostfixUnaryExpression,
  visitPrefixUnaryExpression,
  visitPropertyAccessExpression,
  visitTemplateExpression,
  visitTypeOfExpression,
  visitVoidExpression,
} from "./expression-handlers";
import {
  visitArrowFunction,
  visitFunctionDeclaration,
  visitFunctionExpression,
} from "./function-handlers";
import {
  visitFalseKeyword,
  visitIdentifier,
  visitNoSubstitutionTemplateLiteral,
  visitNullKeyword,
  visitNumericLiteral,
  visitStringLiteral,
  visitThisKeyword,
  visitTrueKeyword,
  visitUndefinedKeyword,
} from "./literal-handlers";
import {
  visitBlock,
  visitBreakStatement,
  visitCatchClause,
  visitContinueStatement,
  visitExpressionStatement,
  visitIfStatement,
  visitLabeledStatement,
  visitReturnStatement,
  visitSourceFile,
  visitThrowStatement,
  visitTryStatement,
  visitVariableStatement,
  visitYieldExpression,
} from "./statement-handlers";

export interface VisitContext {
    isMainContext: boolean;
    isInsideFunction: boolean;
    isFunctionBody: boolean;
    isAssignmentOnly?: boolean;
    exceptionName?: string;
    isInsideTryCatchLambda?: boolean;
    isInsideGeneratorFunction?: boolean;
    isInsideAsyncFunction?: boolean;
    hasReturnedFlag?: string;
    isBracketNotationPropertyAccess?: boolean;
    isObjectLiteralExpression?: boolean;
    currentLabel?: string;
    switchBreakLabel?: string;
    topLevelScopeSymbols: DeclaredSymbols;
    localScopeSymbols: DeclaredSymbols;
    derefBeforeAssignment?: boolean;
    superClassVar?: string;
    lambdaName?: string;
}

export function visit(
    this: CodeGenerator,
    node: Node,
    context: VisitContext,
): string {
    if (ts.isFunctionDeclaration(node)) {
        return visitFunctionDeclaration.call(this, node, context);
    }

    switch (node.kind) {
        case ts.SyntaxKind.ArrowFunction:
            return visitArrowFunction.call(
                this,
                node as ts.ArrowFunction,
                context,
            );
        case ts.SyntaxKind.FunctionExpression:
            return visitFunctionExpression.call(
                this,
                node as ts.FunctionExpression,
                context,
            );
        case ts.SyntaxKind.ClassDeclaration:
            return visitClassDeclaration.call(
                this,
                node as ts.ClassDeclaration,
                context,
            );
        case ts.SyntaxKind.SourceFile:
            return visitSourceFile.call(this, node as ts.SourceFile, context);
        case ts.SyntaxKind.Block:
            return visitBlock.call(this, node as ts.Block, context);
        case ts.SyntaxKind.VariableStatement:
            return visitVariableStatement.call(
                this,
                node as ts.VariableStatement,
                context,
            );
        case ts.SyntaxKind.VariableDeclarationList:
            return visitVariableDeclarationList.call(
                this,
                node as ts.VariableDeclarationList,
                context,
            );
        case ts.SyntaxKind.VariableDeclaration:
            return visitVariableDeclaration.call(
                this,
                node as ts.VariableDeclaration,
                context,
            );
        case ts.SyntaxKind.ObjectLiteralExpression:
            return visitObjectLiteralExpression.call(
                this,
                node as ts.ObjectLiteralExpression,
                context,
            );
        case ts.SyntaxKind.ArrayLiteralExpression:
            return visitArrayLiteralExpression.call(
                this,
                node as ts.ArrayLiteralExpression,
                context,
            );
        case ts.SyntaxKind.ForStatement:
            return visitForStatement.call(
                this,
                node as ts.ForStatement,
                context,
            );
        case ts.SyntaxKind.ForInStatement:
            return visitForInStatement.call(
                this,
                node as ts.ForInStatement,
                context,
            );
        case ts.SyntaxKind.ForOfStatement:
            return visitForOfStatement.call(
                this,
                node as ts.ForOfStatement,
                context,
            );
        case ts.SyntaxKind.WhileStatement:
            return visitWhileStatement.call(
                this,
                node as ts.WhileStatement,
                context,
            );
        case ts.SyntaxKind.DoStatement:
            return visitDoStatement.call(
                this,
                node as ts.DoStatement,
                context,
            );
        case ts.SyntaxKind.SwitchStatement:
            return visitSwitchStatement.call(
                this,
                node as ts.SwitchStatement,
                context,
            );
        case ts.SyntaxKind.CaseClause:
            return visitCaseClause.call(
                this,
                node as ts.CaseClause,
                context,
            );
        case ts.SyntaxKind.DefaultClause:
            return visitDefaultClause.call(
                this,
                node as ts.DefaultClause,
                context,
            );
        case ts.SyntaxKind.BreakStatement:
            return visitBreakStatement.call(
                this,
                node as ts.BreakStatement,
                context,
            );
        case ts.SyntaxKind.ContinueStatement:
            return visitContinueStatement.call(
                this,
                node as ts.ContinueStatement,
                context,
            );
        case ts.SyntaxKind.LabeledStatement:
            return visitLabeledStatement.call(
                this,
                node as ts.LabeledStatement,
                context,
            );
        case ts.SyntaxKind.IfStatement:
            return visitIfStatement.call(this, node as ts.IfStatement, context);
        case ts.SyntaxKind.PrefixUnaryExpression:
            return visitPrefixUnaryExpression.call(
                this,
                node as ts.PrefixUnaryExpression,
                context,
            );
        case ts.SyntaxKind.PostfixUnaryExpression:
            return visitPostfixUnaryExpression.call(
                this,
                node as ts.PostfixUnaryExpression,
                context,
            );
        case ts.SyntaxKind.ParenthesizedExpression:
            return visitParenthesizedExpression.call(
                this,
                node as ts.ParenthesizedExpression,
                context,
            );
        case ts.SyntaxKind.PropertyAccessExpression:
            return visitPropertyAccessExpression.call(
                this,
                node as ts.PropertyAccessExpression,
                context,
            );
        case ts.SyntaxKind.ElementAccessExpression:
            return visitElementAccessExpression.call(
                this,
                node as ts.ElementAccessExpression,
                context,
            );
        case ts.SyntaxKind.ExpressionStatement:
            return visitExpressionStatement.call(
                this,
                node as ts.ExpressionStatement,
                context,
            );
        case ts.SyntaxKind.BinaryExpression:
            return visitBinaryExpression.call(
                this,
                node as ts.BinaryExpression,
                context,
            );
        case ts.SyntaxKind.ConditionalExpression:
            return visitConditionalExpression.call(
                this,
                node as ts.ConditionalExpression,
                context,
            );
        case ts.SyntaxKind.DeleteExpression:
            return visitDeleteExpression.call(
                this,
                node as ts.DeleteExpression,
                context,
            );
        case ts.SyntaxKind.ThrowStatement:
            return visitThrowStatement.call(
                this,
                node as ts.ThrowStatement,
                context,
            );
        case ts.SyntaxKind.TryStatement:
            return visitTryStatement.call(
                this,
                node as ts.TryStatement,
                context,
            );
        case ts.SyntaxKind.CatchClause:
            return visitCatchClause.call(
                this,
                node as ts.CatchClause,
                context,
            );
        case ts.SyntaxKind.CallExpression:
            return visitCallExpression.call(
                this,
                node as ts.CallExpression,
                context,
            );
        case ts.SyntaxKind.YieldExpression:
            return visitYieldExpression.call(
                this,
                node as ts.YieldExpression,
                context,
            );
        case ts.SyntaxKind.ReturnStatement:
            return visitReturnStatement.call(
                this,
                node as ts.ReturnStatement,
                context,
            );
        case ts.SyntaxKind.Identifier:
            return visitIdentifier.call(this, node as ts.Identifier);
        case ts.SyntaxKind.NumericLiteral:
            return visitNumericLiteral.call(this, node as ts.NumericLiteral);
        case ts.SyntaxKind.StringLiteral:
            return visitStringLiteral.call(this, node as ts.StringLiteral);
        case ts.SyntaxKind.NoSubstitutionTemplateLiteral:
            return visitNoSubstitutionTemplateLiteral.call(
                this,
                node as ts.NoSubstitutionTemplateLiteral,
            );
        case ts.SyntaxKind.TemplateExpression:
            return visitTemplateExpression.call(
                this,
                node as ts.TemplateExpression,
                context,
            );
        case ts.SyntaxKind.AwaitExpression:
            return visitAwaitExpression.call(
                this,
                node as ts.AwaitExpression,
                context,
            );
        case ts.SyntaxKind.NewExpression:
            return visitNewExpression.call(
                this,
                node as ts.NewExpression,
                context,
            );
        case ts.SyntaxKind.TypeOfExpression:
            return visitTypeOfExpression.call(
                this,
                node as ts.TypeOfExpression,
                context,
            );
        case ts.SyntaxKind.TrueKeyword:
            return visitTrueKeyword.call(this);
        case ts.SyntaxKind.FalseKeyword:
            return visitFalseKeyword.call(this);
        case ts.SyntaxKind.VoidExpression:
            return visitVoidExpression.call(
                this,
                node as ts.VoidExpression,
                context,
            );
        case ts.SyntaxKind.UndefinedKeyword:
            return visitUndefinedKeyword.call(this);
        case ts.SyntaxKind.NullKeyword:
            return visitNullKeyword.call(this);
        case ts.SyntaxKind.ThisKeyword:
            return visitThisKeyword.call(this);
        default:
            return `/* Unhandled node: ${ts.SyntaxKind[node.kind]} */`;
    }
}
