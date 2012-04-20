/*
    An example interpreter that uses the API provided by gelb (libgel).
    The goal is to show how simple is to use libgel
    to provide scripting to any program based on glib.
    It's still work in progress.
*/

int main(string[] args) {
    if(args.length != 2) {
        printerr("%s requires an argument\n", args[0]);
        return 1;
    }

    ValueArray array;
    try {
        // parsing a file produces an array of values
        array = Gel.parse_file(args[1]);
    }
    catch {
        print("Could not parse file '%s'\n", args[1]);
        return 1;
    }

    // instantiate a context to be used to evaluate
    Gel.Context context = new Gel.Context();

    // insert a function to make it available to the script
    context.insert_function("get-label-from-native",
        (closure, out return_value) => {
            Type label_t = Type.from_name("GtkLabel");
            return_value = Object.new(label_t, "label", "Label made in Vala");
        }
    );

    // insert a value of type string to make it available to the script
    context.insert("title", "Hello Gtk from Gel");

    // for each value obtained during the parsing:
    foreach(Value iter_value in array.values) {
        // print a representation of the value to be evaluated
        print("\n%s ?\n", Gel.Value.to_string(iter_value));

        GLib.Value result_value;
        // if the evaluation yields a value ...
        if(context.eval(iter_value, out result_value))
            // ... then print the value obtained from the evaluation
            print("= %s\n", Gel.Value.to_string(result_value));
    }

    return 0;
}

