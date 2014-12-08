case $PKGCOMPRESSOR in
    "gz")
        PKGEXT=".tar.gz" ;;
    "bzip2")
        PKGEXT=".tar.bz2" ;;
    "xz")
        PKGEXT=".tar.xz" ;;
    "zip")
        PKGEXT=".zip" ;;
    *)
        echo "error: unknown compressor \"$PKGCOMPRESSOR\"" >&2
        exit 1;
esac

which "$PKGCOMPRESSOR" &>/dev/null || { echo "compressor \"$PKGCOMPRESSOR\" not installed"  >&2; exit 1; }

compress()
{
    case $PKGCOMPRESSOR in
        "zip")
            $PKGCOMPRESSOR -q -9 -r - $1 > $2 ;;
        *)
            tar cf - $1 | $PKGCOMPRESSOR -9 - > $2 ;;
    esac
}
