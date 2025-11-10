import ts from "typescript";

import { CodeGenerator } from "./";

export function visitIdentifier(
    this: CodeGenerator,
    node: ts.Identifier,
): string {
    if (node.text === "NaN") {
        return "jspp::AnyValue::make_nan()";
    }

    return node.text;
}

export function visitNumericLiteral(
    this: CodeGenerator,
    node: ts.NumericLiteral,
): string {
    return `jspp::AnyValue::make_number(${this.escapeString(node.text)})`;
}

export function visitStringLiteral(
    this: CodeGenerator,
    node: ts.StringLiteral,
): string {
    return `jspp::AnyValue::make_string("${this.escapeString(node.text)}")`;
}

export function visitNoSubstitutionTemplateLiteral(
    this: CodeGenerator,
    node: ts.NoSubstitutionTemplateLiteral,
): string {
    return `jspp::AnyValue::make_string("${this.escapeString(node.text)}")`;
}

export function visitTrueKeyword(): string {
    return "jspp::AnyValue::make_boolean(true)";
}

export function visitFalseKeyword(): string {
    return "jspp::AnyValue::make_boolean(false)";
}

export function visitUndefinedKeyword(): string {
    return "jspp::AnyValue::make_undefined()";
}

export function visitNullKeyword(): string {
    return "jspp::AnyValue::make_null()";
}
