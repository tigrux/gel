class Demo : Object {
    Gel.Context context;
    ValueArray array;


    Demo() {
        context = new Gel.Context();
        context.insert_function("quit", quit);
        context.insert_object("label", new Gtk.Label("Added from Vala"));
    }

    public static int main(string[] args) {
        Gtk.init(ref args);

        if(args.length != 2) {
            printerr("%s requires an argument\n", args[0]);
            return 1;
        }

        var demo = new Demo();

        try {
            demo.parse(args[1]);
        }
        catch {
            print("Could not parse file '%s'\n", args[1]);
            return 1;
        }

        Gtk.init_add(demo.run);
        Gtk.main();
        return 0;
    }


    void quit(Closure closure, out Value return_value) {
        return_value = "Bye";
        Gtk.main_quit();
    }


    public void parse(string file) throws FileError {
        array = Gel.parse_file(file);
    }


    bool run() {
        foreach(Value iter_value in array.values) {
            print("\n%s ?\n", Gel.Value.to_string(iter_value));

            GLib.Value result_value;
            if(context.eval(iter_value, out result_value))
                print("= %s\n", Gel.Value.to_string(result_value));
        }
        return true;
    }
}


