import ts from "typescript";

import { CodeGenerator } from "./";

export function visitIdentifier(
    this: CodeGenerator,
    node: ts.Identifier,
): string {
    if (node.text === "NaN") {
        return "jspp::JsValue::make_nan()";
    }
    if (node.text === "undefined") {
        return "jspp::JsValue::make_undefined()";
    }

    return node.text;
}

export function visitNumericLiteral(
    this: CodeGenerator,
    node: ts.NumericLiteral,
): string {
    return `jspp::JsValue::make_number(${this.escapeString(node.text)})`;
}

export function visitStringLiteral(
    this: CodeGenerator,
    node: ts.StringLiteral,
): string {
    return `jspp::JsValue::make_string("${this.escapeString(node.text)}")`;
}

export function visitNoSubstitutionTemplateLiteral(
    this: CodeGenerator,
    node: ts.NoSubstitutionTemplateLiteral,
): string {
    return `jspp::JsValue::make_string("${this.escapeString(node.text)}")`;
}

export function visitTrueKeyword(): string {
    return "jspp::JsValue::make_boolean(true)";
}

export function visitFalseKeyword(): string {
    return "jspp::JsValue::make_boolean(false)";
}

export function visitUndefinedKeyword(): string {
    return "jspp::JsValue::make_undefined()";
}

export function visitNullKeyword(): string {
    return "jspp::JsValue::make_null()";
}
