import ts from "typescript";

import { CodeGenerator } from "./";

export function visitIdentifier(
    this: CodeGenerator,
    node: ts.Identifier,
): string {
    if (node.text === "NaN") {
        // return "jspp::AnyValue::make_nan()";
        return "jspp::NaN";
    }
    if (node.text === "undefined") {
        return visitUndefinedKeyword();
    }

    return node.text;
}

export function visitNumericLiteral(
    this: CodeGenerator,
    node: ts.NumericLiteral,
): string {
    if (node.text === "0") return "jspp::ZERO";
    if (node.text === "1") return "jspp::ONE";
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
    // return "jspp::AnyValue::make_boolean(true)";
    return "jspp::TRUE";
}

export function visitFalseKeyword(): string {
    // return "jspp::AnyValue::make_boolean(false)";
    return "jspp::FALSE";
}

export function visitUndefinedKeyword(): string {
    // return "jspp::AnyValue::make_undefined()";
    return "jspp::UNDEFINED";
}

export function visitNullKeyword(): string {
    // return "jspp::AnyValue::make_null()";
    return "jspp::Null";
}

export function visitThisKeyword(this: CodeGenerator): string {
    return `${this.globalThisVar}`;
}
