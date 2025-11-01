import ts from "typescript";

import { CodeGenerator } from "./";

export function visitIdentifier(
    this: CodeGenerator,
    node: ts.Identifier,
): string {
    return node.text;
}

export function visitNumericLiteral(
    this: CodeGenerator,
    node: ts.NumericLiteral,
): string {
    return `jspp::Object::make_number(${this.escapeString(node.text)})`;
}

export function visitStringLiteral(
    this: CodeGenerator,
    node: ts.StringLiteral,
): string {
    return `jspp::Object::make_string("${this.escapeString(node.text)}")`;
}

export function visitNoSubstitutionTemplateLiteral(
    this: CodeGenerator,
    node: ts.NoSubstitutionTemplateLiteral,
): string {
    return `jspp::Object::make_string("${this.escapeString(node.text)}")`;
}

export function visitTrueKeyword(): string {
    return "jspp::Object::make_boolean(true)";
}

export function visitFalseKeyword(): string {
    return "jspp::Object::make_boolean(false)";
}

export function visitUndefinedKeyword(): string {
    return "undefined";
}

export function visitNullKeyword(): string {
    return "null";
}
