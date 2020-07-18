open TestFramework;

let win = Sys.win32 || Sys.cygwin;

// let channel_to_string = channel => {
//   // let buf = Buffer.create(in_channel_length(channel));
//   let buf = Buffer.create(1024);

//   try(
//     while (true) {
//       let line = input_line(channel);
//       Buffer.add_string(buf, line);
//       Buffer.add_char(buf, '\n');
//     }
//   ) {
//   | End_of_file => close_in(channel)
//   };
//   Buffer.contents(buf);
// };

// let channel_to_channel = (read_channel, write_channel) => {
//   set_binary_mode_in(read_channel, true);
//   set_binary_mode_out(write_channel, true);
//   try(
//     while (true) {
//       let byte = input_binary_int(read_channel);
//       output_binary_int(stdout, byte);
//       output_binary_int(write_channel, byte);
//     }
//   ) {
//   | End_of_file => close_out(write_channel)
//   };
// };

let refmt_path = "./node_modules/bs-platform/darwin/refmt.exe";
let ppx_path = "../_build/default/src/bucklescript_bin/bin.exe";

let rec really_read = (fd, ~buf, ~start=0, ~length=1024, ()) =>
  if (length <= 0) {
    ();
  } else {
    let bytes = Bytes.create(length);
    let result = Unix.read(fd, bytes, start, length);
    switch (result) {
    | r when r < length =>
      Buffer.add_subbytes(buf, bytes, 0, r);
      Unix.close(fd);
    | r =>
      Buffer.add_subbytes(buf, bytes, 0, r);
      really_read(fd, ~buf, ~start=0, ~length, ());
    };
  };

let run_ppx = (path, opts) => {
  let (in_read, in_write) = Unix.pipe(~cloexec=true, ());
  let (err_read, err_write) =
    try(Unix.pipe(~cloexec=true, ())) {
    | e =>
      Unix.close(in_read);
      Unix.close(in_write);
      raise(e);
    };
  let (in_read_2, in_write_2) =
    try(Unix.pipe(~cloexec=true, ())) {
    | e =>
      Unix.close(in_read);
      Unix.close(in_write);
      Unix.close(err_read);
      Unix.close(err_write);
      raise(e);
    };
  let (in_read_3, in_write_3) =
    try(Unix.pipe(~cloexec=true, ())) {
    | e =>
      Unix.close(in_read);
      Unix.close(in_write);
      Unix.close(err_read);
      Unix.close(err_write);
      Unix.close(in_read_2);
      Unix.close(in_write_2);
      raise(e);
    };

  let _ =
    try({
      let _ =
        Unix.create_process(
          refmt_path,
          [|"", "--parse=re", "--print", "binary", path|],
          Unix.stdin,
          in_write,
          err_write,
        );
      let output_opts =
        win ? [|"-", "-o", "-"|] : [|"/dev/stdin", "/dev/stdout"|];

      let _ =
        Unix.create_process(
          ppx_path,
          Array.concat([
            [|"", "-schema", "../graphql_schema.json"|],
            opts,
            output_opts,
          ]),
          in_read,
          in_write_2,
          err_write,
        );
      let _ =
        Unix.create_process(
          refmt_path,
          [|"", "--parse=binary"|],
          in_read_2,
          in_write_3,
          err_write,
        );

      ();
    }) {
    | e =>
      Unix.close(in_write);
      Unix.close(in_read);
      Unix.close(err_write);
      Unix.close(in_write_2);
      Unix.close(in_read_2);
      Unix.close(in_write_3);
      raise(e);
    };
  Unix.close(in_write);
  Unix.close(err_write);
  Unix.close(in_read);
  Unix.close(in_write_2);
  Unix.close(in_read_2);
  Unix.close(in_write_3);

  let output_buf = Buffer.create(1024);
  let error_buf = Buffer.create(1024);
  really_read(in_read_3, ~buf=output_buf, ());
  really_read(err_read, ~buf=error_buf, ());
  let output = Buffer.contents(output_buf);
  let error = Buffer.contents(error_buf);

  (output, error);
};

let bsb_path = "./node_modules/bs-platform/darwin/bsc.exe";
let run_bsb = (~ppxOptions, ~filename, ~pathIn) => {
  let (out_read, out_write) = Unix.pipe(~cloexec=true, ());
  let (err_read, err_write) =
    try(Unix.pipe(~cloexec=true, ())) {
    | e =>
      Unix.close(out_read);
      Unix.close(out_write);
      raise(e);
    };
  let _ =
    try({
      let _ =
        Unix.create_process(
          bsb_path,
          [|
            "",
            "-I",
            "./utilities",
            "-c",
            "-w",
            "-30",
            "-ppx",
            ppx_path
            ++ " "
            ++ Array.fold_left(
                 (acc, ppxOption) => (acc == "" ? "" : " ") ++ ppxOption,
                 "",
                 ppxOptions,
               ),
            pathIn ++ "/" ++ filename,
          |],
          Unix.stdin,
          out_write,
          err_write,
        );
      ();
    }) {
    | e =>
      Unix.close(out_write);
      Unix.close(err_write);
      raise(e);
    };
  Unix.close(out_write);
  Unix.close(err_write);

  let output_buf = Buffer.create(1024);
  let error_buf = Buffer.create(1024);
  really_read(out_read, ~buf=output_buf, ());
  really_read(err_read, ~buf=error_buf, ());
  let output = Buffer.contents(output_buf);
  let error = Buffer.contents(error_buf);

  (output, error);
};

let filenames =
  Sys.readdir("operations")
  |> Array.to_list
  |> List.filter(name => {
       switch (String.split_on_char('.', name) |> List.rev) {
       | ["re", _] => true
       | _ => false
       }
     });

let error_filenames =
  Sys.readdir("operations/errors")
  |> Array.to_list
  |> List.filter(name => {
       switch (String.split_on_char('.', name) |> List.rev) {
       | ["re", _] => true
       | _ => false
       }
     });

let runOperations = (describe, filenames, options) => {
  filenames
  |> List.iter(filename => {
       describe(filename, ({test, _}) => {
         test("output", ({expect, _}) => {
           let (output, error) = run_ppx("operations/" ++ filename, options);
           expect.string(output).toMatchSnapshot();
           expect.string(error).toEqual("");
         })
       })
     });
};

let runCompilation = (describe, filenames, ppxOptions) => {
  filenames
  |> List.iter(filename => {
       describe(
         filename,
         ({test, _}) => {
           let (output, error) =
             run_bsb(~ppxOptions, ~filename, ~pathIn="operations");
           test("output", ({expect, _}) => {
             expect.string(output).toMatchSnapshot();
             expect.string(error).toEqual("");
           });
         },
       )
     });
};

type ppxConfig = {
  name: string,
  options: array(string),
};
let ppxConfigs = [
  {name: "Records", options: [||]},
  {name: "Objects", options: [|"-objects"|]},
  {name: "Template", options: [|"-template-tag-location=gql"|]},
  {name: "Legacy", options: [|"-legacy"|]},
  {name: "Apollo", options: [|"-apollo-mode"|]},
];

ppxConfigs
|> List.iter(({name, options}) => {
     describe("Generate " ++ name, ({describe, _}) => {
       filenames
       |> List.iter(filename => {
            describe(filename, ({test, _}) => {
              test("output", ({expect, _}) => {
                let (output, error) =
                  run_ppx("operations/" ++ filename, options);
                expect.string(output).toMatchSnapshot();
                expect.string(error).toEqual("");
              })
            })
          })
     });
     describe("Compile " ++ name, ({describe, _}) => {
       filenames
       |> List.iter(filename => {
            describe(
              filename,
              ({test, _}) => {
                let (output, error) =
                  run_bsb(
                    ~ppxOptions=options,
                    ~filename,
                    ~pathIn="operations",
                  );
                test("output", ({expect, _}) => {
                  expect.string(output).toMatchSnapshot();
                  expect.string(error).toEqual("");
                });
              },
            )
          })
     });
     describe("Error " ++ name, ({describe, _}) =>
       error_filenames
       |> List.iter(filename => {
            describe(
              filename,
              ({test, _}) => {
                let (_, error) =
                  run_bsb(
                    ~ppxOptions=options,
                    ~filename,
                    ~pathIn="operations/errors",
                  );
                test("output", ({expect, _}) => {
                  expect.string(error).toMatchSnapshot()
                });
              },
            )
          })
     );
   });