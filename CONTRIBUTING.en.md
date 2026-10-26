# Contributing

[Русский](CONTRIBUTING.md) | English

Thank you for wanting to help Gran UI! Bug reports, tests, fixes, sample markup and translations are equally valuable. By participating, you agree to the [Code of Conduct](CODE_OF_CONDUCT.en.md). Report vulnerabilities privately — see [SECURITY.en.md](SECURITY.en.md).

## Where

The main repository is [GitHub](https://github.com/ErmaleyAlekov/Gran-UI), the mirror is [Gitverse](https://gitverse.ru/ermaleyalekov/Gran-UI). Issues and pull requests are accepted on both platforms; the maintainer transfers accepted changes to both.

## Bug Report

- The engine commit, OS, compiler, GPU and driver version.
- Minimal markup that reproduces the bug, the expected and the actual result.
- For rendering bugs — a frame from `gran_snapshot <markup.xml> <frame.png>` and, if you have it, how the same looks in a browser with HTML + CSS.
- If the bug is in XML parsing or the schema, it may belong to [Beresta](https://github.com/ErmaleyAlekov/beresta).

## Code Changes

1. Fork and branch from `main`.
2. Build and run the tests — instructions in [docs/building.en.md](docs/building.en.md):

```bash
make test
```

3. For rendering changes compare the frames of the `samples/` markup before and after (`gran_snapshot`) and check a live window with the debug layer (`gran_gallery --debug`): there must be no layer messages.
4. Add a test for the fixed bug or the new feature: a unit test in `tests/src/<module>/` and its registration (see [docs/development.en.md](docs/development.en.md#tests-and-frame-validation)).
5. Open a pull request into `main`: what changed, why and how it was checked.

## Code Rules

The project follows strict coding rules — they are described in [docs/development.en.md](docs/development.en.md#coding-rules), together with the GPU rules and how to add a new element. Every pull request is reviewed by the maintainer personally.

## Documentation

Every document exists in Russian (`*.md`) and English (`*.en.md`); when you change one, update the other.

## License

By submitting changes you agree that they are distributed under the project license — see [LICENSE](LICENSE).
