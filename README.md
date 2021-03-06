**NOTE: This project is currently not being maintained.** If you're interested
in taking over maintenance, please contact the [development mailing
list][startkladde-devel].

If you have patches, please add them as [pull request][pulls]. You should still
report [issues][issues], but don't expect any activity before new maintainers are
found. You could also look if there is any activity in forked repositories using
Github's [network view](https://github.com/startkladde/startkladde/network).

# Startkladde

*Startkladde* is a software for logging flight movements on small airfields or
glider sites and automatically generating statistics and log books. You can run
it on Linux or Windows.

*Startkladde* development has been started by
[Akaflieg Karlsruhe](//www.akaflieg.uni-karlsruhe.de/) for
[LSG Rheinstetten](http://www.lsg-rheinstetten.de) airfield in 2004. Since then
it's being used on many other airfields around the world.

*Startkladde* has two major components:
* [startkladde](//github.com/startkladde/startkladde), the main program for logging flight movements
* [sk_web](//github.com/startkladde/sk_web), the web interface for acessing data on the local network or internet.

## Getting the source

The *Startkladde* source code is managed with [git](http://www.git-scm.com/).
It can be downloaded with the following command:

    $ git clone git://github.com/startkladde/startkladde.git

For more information, please refer to the [git documentation](http://git-scm.com/documentation).

## Installation and Setup

*Startkladde* is based on QT4 and depends on the following major components:

TODO

The process of installing these components and setting up an environment for
development is described in the [BUILDING.md](BUILDING.md) file (TODO).

## Contact and Contributing

You can reach us on our [mailing lists](http://sourceforge.net/mail/?group_id=123075)
* startkladde-users - for users ([subscribe][startkladde-users], [archive](http://sourceforge.net/mailarchive/forum.php?forum_name=startkladde-users))
* startkladde-devel - for developers ([subscribe][startkladde-devel], [archive](http://sourceforge.net/mailarchive/forum.php?forum_name=startkladde-devel))

Bugs and feature request can be submitted on
[Github][issues]

Patches should be submitted using the
[Pull Request][pulls] system of
GitHub.
You can also send patches via mail to the *startkladde-devel* mailing list though.

Here are a few guidelines for creating patches:

- patches should be self-contained
- patches should be self-documenting
  (add a good description on what is changed, and why you are changing it)
- write one patch for one change

## License

You can find the full license text in the [LICENSE](LICENSE) file.

[startkladde-devel]: https://lists.sourceforge.net/lists/listinfo/startkladde-devel
[startkladde-users]: https://lists.sourceforge.net/lists/listinfo/startkladde-users
[issues]: https://github.com/startkladde/startkladde/issues
[pulls]: https://github.com/startkladde/startkladde/pulls
