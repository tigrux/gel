[CCode (cheader_filename = "gel.h")]
namespace Gel {

    [CCode (instance_pos = -1)]
    public delegate void Function(
        GLib.Closure closure,
        out GLib.Value return_value,
        [CCode (array_length_pos = 2.9, array_length_type = "guint")]
        GLib.Value[] param_values,
        Gel.Context? invocation_context);

    public errordomain ContextError {
       ARGUMENTS,
       SYMBOL,
       TYPE,
       PROPERTY,
       INDEX,
       KEY,
       ARITHMETIC,
       LOGIC
    }

    [CCode (type_id = "GEL_TYPE_CONTEXT")]
    [Compact]
    public class Context {
        public Context();
        public Context.with_outer(Gel.Context outer);
        public unowned GLib.Value lookup(string name);
        public unowned Context outer {get;}
        public GLib.List<unowned string> variables {get;}
        public void insert(string name, owned GLib.Value? value);
        public void insert_object(string name, owned GLib.Object object);
        public void insert_function(string name, Gel.Function function);
        public bool remove(string name);
        public bool eval(GLib.Value value, out GLib.Value dest_value) throws ContextError;

        bool gel_context_error();
        void gel_context_set_error(GLib.Error error);
        void gel_context_clear_error();
    }

    public errordomain ParseError {
        SCANNER,
        UNMATCHED_DELIM,
        UNEXPECTED_DELIM,
        EXPECTED_DELIM,
        UNKNOWN_TOKEN
    }

    public GLib.ValueArray parse_file(string file) throws GLib.FileError, ParseError;
    public GLib.ValueArray parse_text(string text, uint text_len) throws ParseError;

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

