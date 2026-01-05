export enum DeclarationType {
    var = "var",
    let = "let",
    const = "const",
    function = "function",
    class = "class",
}

export class DeclaredSymbol {
    type: DeclarationType;
    checked: {
        initialized: boolean;
    };
    func?: { selfName?: string };

    constructor(type: DeclarationType) {
        this.type = type;
        this.checked = {
            initialized: false,
        };
    }

    get isMutable() {
        return this.type === DeclarationType.let ||
            this.type === DeclarationType.var;
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
        checked?: Partial<DeclaredSymbol["checked"]>;
        func?: Partial<DeclaredSymbol["func"]>;
    }) {
        const symbol = new DeclaredSymbol(value.type);
        symbol.checked = {
            ...symbol.checked,
            ...value.checked,
        };
        symbol.func = {
            ...symbol.func,
            ...value.func,
        };
        return this.symbols.set(name, symbol);
    }

    update(
        name: string,
        update: Partial<
            {
                type: DeclaredSymbol["type"];
                checked: Partial<DeclaredSymbol["checked"]>;
                func: Partial<DeclaredSymbol["func"]>;
            }
        >,
    ) {
        const symbol = this.get(name);
        if (symbol) {
            symbol.type = update.type ?? symbol.type;
            symbol.checked = {
                ...symbol.checked,
                ...update.checked,
            };
            symbol.func = {
                ...symbol.func,
                ...update.func,
            };
            return this.symbols.set(name, symbol);
        }
    }
}
