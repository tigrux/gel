[CCode (cheader_filename = "gel.h")]
namespace Gel {

    [CCode (instance_pos = -1)]
    public delegate void Function(
        GLib.Closure closure,
        out GLib.Value return_value,
        [CCode (array_length_pos = 2.9, array_length_type = "guint")]
        GLib.Value[] param_values,
        Gel.Context? invocation_context);

    public struct ArrayIter
    {
        public unowned GLib.Value? next_value();
    }

    [Compact]
    public class Array {
        public Array(uint n_prealloced);
        public uint get_n_values();
        public void set_n_values(uint new_size);
        [CCode (array_length = false)]
        public unowned GLib.Value[] get_values();
        public unowned Array append(GLib.Value value);
        public unowned Array remove(uint index);
        public unowned Array sort(GLib.CompareFunc compare_func);
        public ArrayIter iterator();
    }

    public errordomain ParserError {
        UNKNOWN,
        UNEXP_EOF,
        UNEXP_EOF_IN_STRING,
        UNEXP_EOF_IN_COMMENT,
        NON_DIGIT_IN_CONST,
        DIGIT_RADIX,
        FLOAT_RADIX,
        FLOAT_MALFORMED,
        UNEXP_DELIM,
        UNEXP_EOF_IN_ARRAY,
        UNKNOWN_TOKEN,
        MACRO_MALFORMED,
        MACRO_ARGUMENTS
    }

    [Compact]
    public class Parser {
        public Parser();
        public void input_text(string text, size_t text_len);
        public void input_file(int fd);
        public bool next_value(out GLib.Value value) throws ParserError;
        public Gel.Array get_values() throws ParserError;
    }

    public errordomain ContextError {
       ARGUMENTS,
       UNKNOWN_SYMBOL,
       SYMBOL_EXISTS,
       TYPE,
       PROPERTY,
       INDEX,
       KEY
    }

    [Compact]
    public class Context {
        public Context();
        public Context.with_outer(Gel.Context outer);
        public Context dup();
        public unowned GLib.Value lookup(string name);
        public void define(string name, GLib.Type type, ...);
        public void define_value(string name, owned GLib.Value? value);
        public void define_object(string name, owned GLib.Object object);
        public void define_function(string name, Gel.Function function);
        public bool remove(string name);
        public bool eval(GLib.Value value, out GLib.Value dest_value) throws ContextError;

        bool gel_context_error();
        void clear_error();
    }

    namespace Value {
        bool copy(GLib.Value src_value, out GLib.Value dest_value);
        string repr(GLib.Value? value);
        string to_string(GLib.Value? value);
        bool to_boolean(GLib.Value? value);

    }

    namespace Values {
        public bool add(GLib.Value v1, GLib.Value v2, out GLib.Value dest_value);
        public bool sub(GLib.Value v1, GLib.Value v2, out GLib.Value dest_value);
        public bool mul(GLib.Value v1, GLib.Value v2, out GLib.Value dest_Value);
        public bool div(GLib.Value v1, GLib.Value v2, out GLib.Value dest_value);
        public bool mod(GLib.Value v1, GLib.Value v2, out GLib.Value dest_value);

        public int cmp(GLib.Value v1, GLib.Value v2);

        public int gt(GLib.Value v1, GLib.Value v2);
        public int ge(GLib.Value v1, GLib.Value v2);
        public int eq(GLib.Value v1, GLib.Value v2);
        public int le(GLib.Value v1, GLib.Value v2);
        public int lt(GLib.Value v1, GLib.Value v2);
        public int ne(GLib.Value v1, GLib.Value v2);
    }

}

