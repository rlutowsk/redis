#
# redis: connect to redis database
#
# This file contains package meta data. For additional information on
# the meaning and correct usage of these fields, please consult the
# manual of the "Example" package as well as the comments in its
# PackageInfo.g file.
#
SetPackageInfo( rec(

PackageName := "redis",
Subtitle := "connect to redis database",
Version := "0.1",
Date := "02/07/2026", # dd/mm/yyyy format
License := "GPL-2.0-or-later",

Persons := [
  rec(
    FirstNames := "Rafal",
    LastName := "Lutowski",
    WWWHome := "https://mat.ug.edu.pl/~rlutowsk",
    Email := "rafal.lutowski@ug.edu.pl",
    IsAuthor := true,
    IsMaintainer := true,
    PostalAddress := "80-308",
    Place := "Gdansk",
    Institution := "University of Gdansk",
  ),
],

SourceRepository := rec(
    Type := "git",
    URL := "https://github.com/rlutowsk/redis",
),
IssueTrackerURL := Concatenation( ~.SourceRepository.URL, "/issues" ),
PackageWWWHome  := "https://rlutowsk.github.io/redis/",
PackageInfoURL  := Concatenation( ~.PackageWWWHome, "PackageInfo.g" ),
README_URL      := Concatenation( ~.PackageWWWHome, "README.md" ),
ArchiveURL      := Concatenation( ~.SourceRepository.URL,
                                 "/releases/download/v", ~.Version,
                                 "/", ~.PackageName, "-", ~.Version ),

ArchiveFormats := ".tar.gz",

AbstractHTML   :=  "",

PackageDoc := rec(
  BookName  := "redis",
  ArchiveURLSubset := ["doc"],
  HTMLStart := "doc/chap0_mj.html",
  PDFFile   := "doc/manual.pdf",
  SixFile   := "doc/manual.six",
  LongTitle := "connect to redis database",
),

Dependencies := rec(
  GAP := ">= 4.13",
  NeededOtherPackages := [ ],
  SuggestedOtherPackages := [ ],
  ExternalConditions := [ ],
),

AvailabilityTest := function()
  if not IsKernelExtensionAvailable("redis") then
    LogPackageLoadingMessage(PACKAGE_WARNING,
                             "failed to load kernel module of package redis");
    return false;
  fi;
  return true;
end,

TestFile := "tst/testall.g",

#Keywords := [ "TODO" ],

));
