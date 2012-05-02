/*
    An example interpreter that uses the API provided by Gel (libgel).
    The goal is to show how simple is to use libgel
    to provide scripting to any program based on glib.
    It's still work in progress.
*/

int main(string[] args) {

    if(args.length != 2) {
        printerr("%s requires an argument\n", args[0]);
        return 1;
    }

    ValueArray parsed_array;
    try {
        // parsing a file returns an array of values
        parsed_array = Gel.parse_file(args[1]);
    }
    catch(FileError error) {
        print("Error reading '%s'\n", args[1]);
        print("%s\n", error.message);
        return 1;
    }
    catch(Gel.ParseError error) {
        print("Error parsing '%s'\n", args[1]);
        print("%s\n", error.message);
        return 1;
    }

    // instantiate a context to be used to evaluate the parsed values
    Gel.Context context = new Gel.Context();

    // insert a function to make it available to the script
    context.insert_function("make-label",
        (closure, out return_value) => {
            Type label_t = Type.from_name("GtkLabel");
            return_value = Object.new(label_t, "label", "Label made in Vala");
        }
    );

    // insert a value of type string to make it available to the script
    context.insert("title", "Hello Gtk from Gel");

    // for each value obtained during the parsing:
    foreach(Value iter_value in parsed_array.values) {
        // print a representation of the value to be evaluated
        print("\n%s ?\n", Gel.Value.repr(iter_value));

        try {
            GLib.Value result_value;
            // if the evaluation yields a value ...
            if(context.eval(iter_value, out result_value))
                // ... then print the value obtained from the evaluation
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

