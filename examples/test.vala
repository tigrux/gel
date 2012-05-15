/*
    An example interpreter that uses the API provided by Gel (libgel).
    The goal is to show how simple is to use libgel
    to provide scripting to any program based on glib.
    It's still work in progress.
*/


/* this function will be made available to the script */
void make_label(GLib.Closure closure, out GLib.Value return_value)
{
    Type label_t = Type.from_name("GtkLabel");
    return_value = Object.new(label_t, "label", "Label made in Vala");
}


int main(string[] args) {
    if(args.length != 2) {
        printerr("%s requires an argument\n", args[0]);
        return 1;
    }

    string content;
    size_t content_len;

    try
    {
        GLib.FileUtils.get_contents(args[1], out content, out content_len);
    }
    catch(FileError error) {
        print("Error reading '%s'\n", args[1]);
        print("%s\n", error.message);
        return 1;
    }

    Gel.Parser parser = new Gel.Parser();
    parser.input_text(content, content_len);

    Gel.Array array;
    try {
        array = parser.get_values();
    }
    catch(Gel.ParserError error) {
        print("Error parsing '%s'\n", args[1]);
        print("%s\n", error.message);
        return 1;
    }

    Gel.Context context = new Gel.Context();
    context.define_function("make-label", make_label);
    context.define("title", typeof(string), "Hello Gtk from Gel");

    foreach(Value value in array) {
        print("\n%s ?\n", Gel.Value.repr(value));
        try {
            GLib.Value result_value;
            if(context.eval(value, out result_value))
                print("= %s\n", Gel.Value.to_string(result_value));
        }
        catch(Gel.ContextError error) {
            print("Error evaluating '%s'\n", args[1]);
            print("%s\n", error.message);
            return 1;
        }
    }

    return 0;
}

