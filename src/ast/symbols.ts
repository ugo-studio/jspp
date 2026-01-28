export enum DeclarationType {
    var = "var",
    let = "let",
    const = "const",
    function = "function",
    class = "class",
    enum = "enum",
}

export class DeclaredSymbol {
    type: DeclarationType;
    checks: {
        initialized: boolean;
    };
    func:
        | { nativeName?: string; isAsync?: boolean; isGenerator?: boolean }
        | null;

    constructor(type: DeclarationType) {
        this.type = type;
        this.checks = {
            initialized: false,
        };
        this.func = null;
    }

    get isMutable() {
        return this.type === DeclarationType.let ||
            this.type === DeclarationType.var;
    }

    updateChecked(update: Partial<DeclaredSymbol["checks"]>) {
        this.checks = {
            ...this.checks,
            ...update,
        };
    }

    updateFunc(update: Partial<DeclaredSymbol["func"]>) {
        this.func = update
            ? {
                ...this.func,
                ...update,
            }
            : null;
    }
}

export class DeclaredSymbols {
    private symbols: Map<
        string,
        DeclaredSymbol
    >;

    constructor(...m: DeclaredSymbols[]) {
        this.symbols = new Map();
        m.forEach((ds) => ds.symbols.forEach((v, k) => this.symbols.set(k, v)));
    }

    get names() {
        return new Set(this.symbols.keys());
    }

    has(name: string) {
        return this.symbols.has(name);
    }

    get(name: string) {
        return this.symbols.get(name);
    }

    add(name: string, value: {
        type: DeclaredSymbol["type"];
        checks?: Partial<DeclaredSymbol["checks"]>;
        func?: Partial<DeclaredSymbol["func"]>;
    }) {
        const sym = new DeclaredSymbol(value.type);
        if (value.checks !== undefined) sym.updateChecked(value.checks);
        if (value.func !== undefined) sym.updateFunc(value.func);
        return this.symbols.set(name, sym);
    }

    update(
        name: string,
        update: Partial<
            {
                type: DeclaredSymbol["type"];
                checks: Partial<DeclaredSymbol["checks"]>;
                func: Partial<DeclaredSymbol["func"]>;
            }
        >,
    ) {
        const sym = this.get(name);
        if (sym) {
            if (update.type !== undefined) sym.type = update.type;
            if (update.checks !== undefined) sym.updateChecked(update.checks);
            if (update.func !== undefined) sym.updateFunc(update.func);
            return this.symbols.set(name, sym);
        }
    }
}
