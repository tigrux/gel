/*
    An example interpreter that uses the API provided by Gel (libgel).
    The goal is to show how simple is to use libgel
    to provide scripting to any program based on 
    It's still work in progress.
*/


/* this function will be made available to the script */
void make_label(Closure closure, out Value return_value)
{
    Type label_t = Type.from_name("GtkLabel");
    return_value = Object.new(label_t, "label", "Label made in Vala");
}


int main(string[] args) {
    if(args.length < 2) {
        string program = args[0];
        printerr("%s requires an argument\n", program);
        return 1;
    }

    try {
        string filename = args[1];
        string content;
        size_t content_len;
        FileUtils.get_contents(filename, out content, out content_len);

        Gel.Parser parser = new Gel.Parser();
        parser.input_text(content, content_len);

        Gel.Context context = new Gel.Context();
        context.define("title", typeof(string), "Hello Gtk from Gel");
        context.define_function("make-label", make_label);

        foreach(Value parsed_value in parser) {
            print("\n%s ?\n", Gel.Value.repr(parsed_value));

            Value evaluated_value;
            if(context.eval(parsed_value, out evaluated_value))
                print("= %s\n", Gel.Value.to_string(evaluated_value));
        }
    }
    catch(Error e) {
        print("Error of domain %s\n", e.domain.to_string());
        print("%s\n", e.message);
        return 1;
    }

    return 0;
}

