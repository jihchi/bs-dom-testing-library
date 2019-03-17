open Jest;
open Webapi.Dom;
open Webapi.Dom.Element;

let render = html => {
  let body = Document.createElement("body", document);

  body->setInnerHTML(html);

  document->Document.unsafeAsHtmlDocument->HtmlDocument.setBody(body);

  body;
};

afterEach(() =>
  switch (document->Document.unsafeAsHtmlDocument->HtmlDocument.body) {
  | Some(body) => body->setInnerHTML("")
  | None => raise(Failure("Not document body found"))
  }
);

type parser;

[@bs.new]
external domParser : unit => parser = "DOMParser";

[@bs.send.pipe : parser]
external parseFromString : ( string, [@bs.as "text/html"] _) => Dom.element = "";

[@bs.get]
external body : Dom.element => Dom.element = "";

[@bs.get]
external firstChild : Dom.element => Dom.element = "";

[@bs.get]
external tagName : Dom.element => string = "";

[@bs.val]
external setTimeout : (unit => unit, int) => float = "setTimeout";

describe("DomTestingLibrary", () => {
  open DomTestingLibrary;
  open Expect;

  let div = domParser()
    |> parseFromString({j|
        <div>
          <b title="greeting">Hello,</b>
          <p data-testid="world"> World!</p>
          <input type="text" placeholder="Enter something" />
          <input type="text" value="Some value" />
          <img src="" alt="Alt text" />
        </div>
      |j})
    |> body
    |> firstChild;

  describe("prettyDOM", () => {
    test("works", () => {
      let actual = div |> prettyDOM;

      expect(actual) |> toMatchSnapshot;
    });

    test("works with maxLength", () => {
      let actual = div |> prettyDOM(~maxLength=25);

      expect(actual) |> toMatchSnapshot;
    });
  });

  test("getNodeText works", () =>
    render({|<b title="greeting">Hello,</b>|})
    |> getByTitle("greeting")
    |> getNodeText
    |> expect
    |> toMatchSnapshot
  );

  test("getByTestId works", () => {
    let actual = div |> getByTestId("world");

    expect(actual) |> toMatchSnapshot;
  });

  test("getByPlaceholderText works", () => {
    let actual = div |> getByPlaceholderText("Enter something");

    expect(actual) |> toMatchSnapshot;
  });

  test("getByAltText works", () => {
    let actual = div |> getByAltText("Alt text");

    expect(actual) |> toMatchSnapshot;
  });

  test("getByTitle works", () => {
    let actual = div |> getByTitle("greeting");

    expect(actual) |> toMatchSnapshot;
  });

  test("getByValue works", () => {
    let actual = div |> getByValue("Some value");

    expect(actual) |> toMatchSnapshot;
  });

  describe("getByText", () => {
    test("works with string matcher", () =>
      render({|<b title="greeting">Hello,</b>|})
      |> getByText(~matcher=`Str("Hello,"))
      |> expect
      |> toMatchSnapshot
    );

    test("works with regex matcher", () =>
      render({|<p data-testid="world"> World!</p>|})
      |> getByText(~matcher=`RegExp([%bs.re "/\\w!/"]))
      |> expect
      |> toMatchSnapshot
    );

    test("works with function matcher", () =>
      render({|<p data-testid="world"> World!</p>|})
      |> getByText(~matcher=`Func(( _text, node ) => (node |> tagName) === "P"))
      |> expect
      |> toMatchSnapshot
    );
  });

  describe("wait", () => {
    testPromise("works", () => {
      let number = ref(10);
      let timeout = Js.Math.floor(Js.Math.random() *. 300.);
      let _ = setTimeout(() => { number := 100 }, timeout);
      let callback = () => assert(number^ == 100);

      wait(~callback, ())
        |> Js.Promise.then_(_ => Js.Promise.resolve(pass));
    });

    testPromise("supports timeout option", () => {
      let number = ref(10);
      let _ = setTimeout(() => { number := 100 }, 1000);
      let callback = () => assert(number^ == 100);
      let options = Wait.makeOptions(~timeout=500, ());

      wait(~callback, ~options, ())
        |> Js.Promise.catch(_ => Js.Promise.resolve(pass));
    });

    testPromise("supports interval option", () => {
      let number = ref(0);
      let callback = () => assert(number^ == 10);
      let options = Wait.makeOptions(~interval=10, ~timeout=45, ());

      wait(~callback, ~options, ())
        |> Js.Promise.catch(_ => Js.Promise.resolve(pass));
    });
  });

  describe("FireEvent", () => {
    test("click works", () => {
      let node = document |> Document.createElement("button");
      let spy = JestJs.inferred_fn();
      let fn = spy |> MockJs.fn;
      let clickHandler = _ => [@bs] fn("clicked!") |> ignore;

      node |> Element.addEventListener("click", clickHandler);

      FireEvent.click(node);

      expect(spy |> MockJs.calls) |> toEqual([|"clicked!"|]);
    });

    test("change works", () => {
      let node = document |> Document.createElement("input");
      let spy = JestJs.inferred_fn();
      let fn = spy |> MockJs.fn;
      let changeHandler = _ => [@bs] fn("changed!") |> ignore;
      let event = Event.makeWithOptions("change", { "target": { "value": "1" } });

      node |> Element.addEventListener("change", changeHandler);

      FireEvent.change(node, event);

      expect(spy |> MockJs.calls) |> toEqual([|"changed!"|]);
    });
  });
});
