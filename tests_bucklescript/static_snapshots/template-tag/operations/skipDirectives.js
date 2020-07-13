// Generated by BUCKLESCRIPT, PLEASE EDIT WITH CARE
'use strict';


var Raw = { };

var query = (require("gql")`
  query ($var: Boolean!)  {
    v1: variousScalars  {
      nullableString @skip(if: $var)
      string @skip(if: $var)
    }
    v2: variousScalars  {
      nullableString @include(if: $var)
      string @include(if: $var)
    }
  }
`);

function parse(value) {
  var value$1 = value.v1;
  var value$2 = value$1.nullableString;
  var value$3 = value$1.string;
  var value$4 = value.v2;
  var value$5 = value$4.nullableString;
  var value$6 = value$4.string;
  return {
          v1: {
            nullableString: !(value$2 == null) ? value$2 : undefined,
            string: !(value$3 == null) ? value$3 : undefined
          },
          v2: {
            nullableString: !(value$5 == null) ? value$5 : undefined,
            string: !(value$6 == null) ? value$6 : undefined
          }
        };
}

function serialize(value) {
  var value$1 = value.v2;
  var value$2 = value$1.string;
  var string = value$2 !== undefined ? value$2 : null;
  var value$3 = value$1.nullableString;
  var nullableString = value$3 !== undefined ? value$3 : null;
  var v2 = {
    nullableString: nullableString,
    string: string
  };
  var value$4 = value.v1;
  var value$5 = value$4.string;
  var string$1 = value$5 !== undefined ? value$5 : null;
  var value$6 = value$4.nullableString;
  var nullableString$1 = value$6 !== undefined ? value$6 : null;
  var v1 = {
    nullableString: nullableString$1,
    string: string$1
  };
  return {
          v1: v1,
          v2: v2
        };
}

function serializeVariables(inp) {
  return {
          var: inp.var
        };
}

function makeVariables($$var, param) {
  return {
          var: $$var
        };
}

var Z__INTERNAL = {
  graphql_module: 0
};

var MyQuery = {
  Raw: Raw,
  query: query,
  parse: parse,
  serialize: serialize,
  serializeVariables: serializeVariables,
  makeVariables: makeVariables,
  Z__INTERNAL: Z__INTERNAL
};

exports.MyQuery = MyQuery;
/* query Not a pure module */