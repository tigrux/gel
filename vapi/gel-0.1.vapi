[CCode (cprefix = "Gel", lower_case_cprefix = "gel_", cheader_filename = "gel.h")]
namespace Gel {
    public class Context : GLib.Object {
        public Context();
        public Context.with_outer(Gel.Context outer);
        public unowned GLib.Value find_symbol(string name);
        public Gel.Context outer {get; set construct;}
        public void add_symbol(string name, owned GLib.Value value);
        public void add_object(string name, owned GLib.Object object);
        public void add_default_symbols();
        public bool remove_symbol(string name);
        public bool eval(GLib.Value value, out GLib.Value dest_value);
        public unowned GLib.Value eval_value(GLib.Value value, out GLib.Value tmp_value);
        public signal void quit();
    }

    GLib.ValueArray parse_file(string file) throws GLib.FileError;
    GLib.ValueArray parse_string(string text, uint text_len);

    bool values_add(GLib.Value v1, GLib.Value v2, out GLib.Value dest_value);
    bool values_sub(GLib.Value v1, GLib.Value v2, out GLib.Value dest_value);
    bool values_mul(GLib.Value v1, GLib.Value v2, out GLib.Value dest_Value);
    bool values_div(GLib.Value v1, GLib.Value v2, out GLib.Value dest_value);
    bool values_mod(GLib.Value v1, GLib.Value v2, out GLib.Value dest_value);

    int values_compare(GLib.Value v1, GLib.Value v2);

    bool values_gt(GLib.Value v1, GLib.Value v2);
    bool values_ge(GLib.Value v1, GLib.Value v2);
    bool values_eq(GLib.Value v1, GLib.Value v2);
    bool values_le(GLib.Value v1, GLib.Value v2);
    bool values_lt(GLib.Value v1, GLib.Value v2);
    bool values_ne(GLib.Value v1, GLib.Value v2);
}

