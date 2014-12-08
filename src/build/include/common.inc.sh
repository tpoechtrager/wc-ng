export LC_ALL="C"

case "$(uname -s)" in
  *NT*)
    EXESUFFIX=".exe" ;;
  *)
    EXESUFFIX="" ;;
esac
