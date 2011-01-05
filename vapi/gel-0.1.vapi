[CCode (cprefix = "Gel", lower_case_cprefix = "gel_", cheader_filename = "gel.h")]
namespace Gel {

    [Compact]
    public class Context {
        public Context();
        public Context.with_outer(Gel.Context outer);
        public unowned GLib.Value find_symbol(string name);
        public unowned Context outer {get;}
        public void add_symbol(string name, owned GLib.Value value);
        public void add_object(string name, owned GLib.Object object);
        public void add_function(string name, GLib.Func function);
        public void add_default_symbols();
        public bool remove_symbol(string name);
        public bool eval(GLib.Value value, out GLib.Value dest_value);
        public unowned GLib.Value eval_value(GLib.Value value, out GLib.Value tmp_value);
    }

    GLib.ValueArray parse_file(string file) throws GLib.FileError;
    GLib.ValueArray parse_string(string text, uint text_len);

    namespace Values {
        bool add(GLib.Value v1, GLib.Value v2, out GLib.Value dest_value);
        bool sub(GLib.Value v1, GLib.Value v2, out GLib.Value dest_value);
        bool mul(GLib.Value v1, GLib.Value v2, out GLib.Value dest_Value);
        bool div(GLib.Value v1, GLib.Value v2, out GLib.Value dest_value);
        bool mod(GLib.Value v1, GLib.Value v2, out GLib.Value dest_value);

        int compare(GLib.Value v1, GLib.Value v2);

        bool gt(GLib.Value v1, GLib.Value v2);
        bool ge(GLib.Value v1, GLib.Value v2);
        bool eq(GLib.Value v1, GLib.Value v2);
        bool le(GLib.Value v1, GLib.Value v2);
        bool lt(GLib.Value v1, GLib.Value v2);
        bool ne(GLib.Value v1, GLib.Value v2);
    }
}

