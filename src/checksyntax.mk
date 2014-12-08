ifeq (1,$(CHECKSYNTAX))
  CXXLINKER=/bin/true
  WINDRES=/bin/true
  NOPCH=1
  override CFLAGS+=-fsyntax-only
  override CXXFLAGS+=-fsyntax-only
else
  CXXLINKER=$(CXX)
endif

