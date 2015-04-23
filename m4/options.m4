dnl SYNOPSIS: OPTION_DEFAULT_ENABLE([name], [enable_flag_var])
dnl EXAMPLE: OPTION_DEFAULT_ENABLE([mysql], [ENABLE_MYSQL])
dnl note: supports hyphenated feature names now.
AC_DEFUN([OPTION_DEFAULT_ENABLE], [
AC_ARG_ENABLE($1, [  --disable-$1     Disable the $1 module],
        [       if test "x$enableval" = "xno" ; then
                        disable_]m4_translit([$1], [-+.], [___])[=yes
                        enable_]m4_translit([$1], [-+.], [___])[=no
			AC_MSG_NOTICE([Disable $1 module requested ])
                fi
        ], [ AC_MSG_NOTICE([Disable $1 module NOT requested]) ])
AM_CONDITIONAL([$2], [test "$disable_]m4_translit([$1], [-+.], [___])[" != "yes"])
])

dnl SYNOPSIS: OPTION_DEFAULT_DISABLE([name], [enable_flag_var])
dnl EXAMPLE: OPTION_DEFAULT_DISABLE([mysql], [ENABLE_MYSQL])
dnl note: supports hyphenated feature names now.
AC_DEFUN([OPTION_DEFAULT_DISABLE], [
AC_ARG_ENABLE($1, [  --enable-$1     Enable the $1 module: $3],
        [       if test "x$enableval" = "xyes" ; then
                        enable_]m4_translit([$1], [-+.], [___])[=yes
                        disable_]m4_translit([$1], [-+.], [___])[=no
			AC_MSG_NOTICE([Enable $1 module requested])
                fi
        ], [ AC_MSG_NOTICE([Enable $1 module NOT requested]) ])
AM_CONDITIONAL([$2], [test "$enable_]m4_translit([$1], [-+.], [___])[" == "yes"])
])

dnl SYNOPSIS: OPTION_WITH([name], [VAR_BASE_NAME])
dnl EXAMPLE: OPTION_WITH([xyz], [XYZ])
dnl NOTE: With VAR_BASE_NAME being XYZ, this macro will set XYZ_INCIDR and
dnl 	XYZ_LIBDIR to the include path and library path respectively.
AC_DEFUN([OPTION_WITH], [
AC_ARG_WITH(
	$1,
	AS_HELP_STRING(
		[--with-$1@<:@=path@:>@],
		[Specify $1 path @<:@default=/usr/local@:>@]
	),
	[WITH_$2=$withval
	 AM_CONDITIONAL([ENABLE_$2], [true])
	],
	[WITH_$2=/usr/local]
)

if test -d $WITH_$2/lib; then
	$2_LIBDIR=$WITH_$2/lib
	$2_LIBDIR_FLAG=-L$WITH_$2/lib
fi
if test "x$$2_LIBDIR" = "x"; then
	$2_LIBDIR=$WITH_$2/lib64
	$2_LIBDIR_FLAG=-L$WITH_$2/lib64
fi
if test -d $WITH_$2/lib64; then
	$2_LIB64DIR=$WITH_$2/lib64
	$2_LIB64DIR_FLAG=-L$WITH_$2/lib64
fi
if test -d $WITH_$2/include; then
	$2_INCDIR=$WITH_$2/include
	$2_INCDIR_FLAG=-I$WITH_$2/include
fi
AC_SUBST([$2_LIBDIR], [$$2_LIBDIR])
AC_SUBST([$2_LIB64DIR], [$$2_LIB64DIR])
AC_SUBST([$2_INCDIR], [$$2_INCDIR])
AC_SUBST([$2_LIBDIR_FLAG], [$$2_LIBDIR_FLAG])
AC_SUBST([$2_LIB64DIR_FLAG], [$$2_LIB64DIR_FLAG])
AC_SUBST([$2_INCDIR_FLAG], [$$2_INCDIR_FLAG])
])

dnl SYNOPSIS: OPTION_WITH_PORT([name])
dnl EXAMPLE: OPTION_WITH_PORT([XYZ],[411])
dnl sets default value of XYZPORT, using second argument as value if not given
AC_DEFUN([OPTION_WITH_PORT], [
AC_ARG_WITH(
	$1PORT,
	AS_HELP_STRING(
		[--with-$1PORT@<:@=NNN@:>@],
		[Specify $1 runtime default port @<:@configure default=$2@:>@]
	),
	[$1PORT=$withval],
	[$1PORT=$2; withval=$2]
)
$1PORT=$withval
if printf "%d" "$withval" >/dev/null 2>&1; then
	:
else
	AC_MSG_ERROR([--with-$1PORT given non-integer input $withval])
fi
AC_DEFINE_UNQUOTED([$1PORT],[$withval],[Default port for $1 to listen on])
AC_SUBST([$1PORT],[$$1PORT])
])

dnl SYNOPSIS: OPTION_WITH_OR_BUILD(featurename,buildincdir,buildlibdirs)
dnl REASON: configuring against other subprojects needs a little love.
dnl EXAMPLE: OPTION_WITH_OR_BUILD([sos], [../sos/src])
dnl NOTE: With featurename being sos, this macro will set SOS_INCIDR and
dnl 	SOS_LIBDIR to the include path and library path respectively.
dnl if user does not specify prefix of a prior install, the
dnl source tree at relative location builddir will be assumed useful.
dnl The relative location should have been generated by some configure output
dnl prior to its use here.
AC_DEFUN([OPTION_WITH_OR_BUILD], [
AC_ARG_WITH(
	$1,
	AS_HELP_STRING(
		[--with-$1@<:@=path@:>@],
		[Specify $1 path @<:@default=in build tree@:>@]
	),
	[WITH_]m4_translit([$1], [-+.a-z], [___A-Z])[=$withval
	 AM_CONDITIONAL([ENABLE_]m4_translit([$1], [-+.a-z], [___A-Z])[], [true])
	],
	[WITH_]m4_translit([$1], [-+.a-z], [___A-Z])[=build]
)

[if test "x$WITH_]m4_translit([$1], [-a-z], [_A-Z])[" != "xbuild"; then
	if test -d $WITH_]m4_translit([$1], [-+.a-z], [___A-Z])[/lib; then
		]m4_translit([$1], [-+.a-z], [___A-Z])[_LIBDIR=$WITH_]m4_translit([$1], [-+.a-z], [___A-Z])[/lib
		]m4_translit([$1], [-+.a-z], [___A-Z])[_LIBDIR_FLAG=-L$WITH_]m4_translit([$1], [-+.a-z], [___A-Z])[/lib
	fi
	if test "x$]m4_translit([$1], [-+.a-z], [___A-Z])[_LIBDIR" = "x"; then
		]m4_translit([$1], [-+.a-z], [___A-Z])[_LIBDIR=$WITH_]m4_translit([$1], [-+.a-z], [___A-Z])[/lib64
		]m4_translit([$1], [-+.a-z], [___A-Z])[_LIBDIR_FLAG=-L$WITH_]m4_translit([$1], [-+.a-z], [___A-Z])[/lib64
	fi
	if test -d $WITH_]m4_translit([$1], [-+.a-z], [___A-Z])[/lib64; then
		]m4_translit([$1], [-+.a-z], [___A-Z])[_LIB64DIR=$WITH_]m4_translit([$1], [-+.a-z], [___A-Z])[/lib64
		]m4_translit([$1], [-+.a-z], [___A-Z])[_LIB64DIR_FLAG=-L$WITH_]m4_translit([$1], [-+.a-z], [___A-Z])[/lib64
	fi
	if test -d $WITH_]m4_translit([$1], [-+.a-z], [___A-Z])[/include; then
		]m4_translit([$1], [-+.a-z], [___A-Z])[_INCDIR=$WITH_]m4_translit([$1], [-+.a-z], [___A-Z])[/include
		]m4_translit([$1], [-+.a-z], [___A-Z])[_INCDIR_FLAG=-I$WITH_]m4_translit([$1], [-+.a-z], [___A-Z])[/include
	fi
else
	# sosbuilddir should exist by ldms configure time

	tmpsrcdir=`(cd $srcdir/$2 && pwd)`
	dirlist=""
	if test -n "$3"; then
		for dirtmp in $3; do
			tmpbuilddir=`(cd $dirtmp && pwd)` && tmpflag="$tmpflag -L$tmpbuilddir" && dirlist="$dirlist $tmpbuilddir"
		done
		# no -L without args allowed.
		tmpflag=`echo $tmpflag | sed -e 's%-L %%g'`
	else
		tmpbuilddir=`(cd $2 && pwd)`
		tmpflag="-L$tmpbuilddir"
	fi
	]m4_translit([$1], [-+.a-z], [___A-Z])[_INCDIR=$tmpsrcdir
	]m4_translit([$1], [-+.a-z], [___A-Z])[_INCDIR_FLAG=-I$tmpsrcdir
	]m4_translit([$1], [-+.a-z], [___A-Z])[_LIBDIR=$dirlist
	]m4_translit([$1], [-+.a-z], [___A-Z])[_LIBDIR_FLAG=$tmpflag
	]m4_translit([$1], [-+.a-z], [___A-Z])[_LIB64DIR_FLAG=""
	]m4_translit([$1], [-+.a-z], [___A-Z])[_LIB64DIR=""
fi
]
AC_SUBST(m4_translit([$1], [-+.a-z], [___A-Z])[_LIBDIR], [$]m4_translit([$1], [-+.a-z], [___A-Z])[_LIBDIR])
AC_SUBST(m4_translit([$1], [-+.a-z], [___A-Z])[_LIB64DIR], [$]m4_translit([$1], [-+.a-z], [___A-Z])[_LIB64DIR])
AC_SUBST(m4_translit([$1], [-+.a-z], [___A-Z])[_INCDIR], [$]m4_translit([$1], [-+.a-z], [___A-Z])[_INCDIR])
AC_SUBST(m4_translit([$1], [-+.a-z], [___A-Z])[_LIBDIR_FLAG], [$]m4_translit([$1], [-+.a-z], [___A-Z])[_LIBDIR_FLAG])
AC_SUBST(m4_translit([$1], [-+.a-z], [___A-Z])[_LIB64DIR_FLAG], [$]m4_translit([$1], [-+.a-z], [___A-Z])[_LIB64DIR_FLAG])
AC_SUBST(m4_translit([$1], [-+.a-z], [___A-Z])[_INCDIR_FLAG], [$]m4_translit([$1], [-+.a-z], [___A-Z])[_INCDIR_FLAG])
])

dnl Similar to OPTION_WITH, but a specific case for MYSQL
AC_DEFUN([OPTION_WITH_MYSQL], [
AC_ARG_WITH(
	[mysql],
	AS_HELP_STRING(
		[--with-mysql@<:@=path@:>@],
		[Specify mysql path @<:@default=/usr/local@:>@]
	),
	[	dnl $withval is given.
		WITH_MYSQL=$withval
		mysql_config=$WITH_MYSQL/bin/mysql_config
	],
	[	dnl $withval is not given.
		mysql_config=`which mysql_config`
	]
)

if test $mysql_config
then
	MYSQL_LIBS=`$mysql_config --libs`
	MYSQL_INCLUDE=`$mysql_config --include`
else
	AC_MSG_ERROR([Cannot find mysql_config, please specify
			--with-mysql option.])
fi
AC_SUBST([MYSQL_LIBS])
AC_SUBST([MYSQL_INCLUDE])
])

AC_DEFUN([OPTION_BUILD_LIBEVENT],[
if test "x$LIBEVENT_BUILD" = "x1"; then
	AC_MSG_NOTICE([Generating libevent2 ])
	if test -f libevent-2.0.21-stable.tar.gz; then
		tar zxf libevent-2.0.21-stable.tar.gz
		cd libevent-2.0.21-stable && \
		iprefix=`pwd`/lib/ovis-ldms
		mkdir -p $iprefix
		./configure \
		CFLAGS="-O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -m64 -march=native" \
		--prefix=${iprefix} \
		--datadir=${iprefix}/share \
		--includedir=${iprefix}/include \
		--libdir=${iprefix}/lib \
		--libexecdir=${iprefix}/libexec \
		--mandir=${iprefix}/share/man \
		--infodir=${iprefix}/share/info \
		--disable-dependency-tracking \
		--disable-static &&
		make install
		with_libevent=$iprefix	
		cd ..
	else
		AC_MSG_ERROR([cannot find libevent-2.0.21-stable.tar.gz ])
	fi
else
	AC_MSG_NOTICE([Assuming external libevent2 ])
fi
])

dnl this could probably be generalized for handling lib64,lib python-binding issues
AC_DEFUN([OPTION_WITH_EVENT],[
  AC_ARG_WITH([libevent],
  [  --with-libevent=DIR      use libevent in DIR],
  [ case "$withval" in
    yes|no)
      AC_MSG_RESULT(no)
      ;;
    *)
      if test "x$withval" != "x/usr"; then
        dnl CPPFLAGS="-I$withval/include"
	if test -f $withval/lib/libevent.so; then
	  libeventpath=$withval/lib
          EVENTLIBS="-L$withval/lib"
        fi 
	if test -f $withval/lib64/libevent.so; then
          EVENTLIBS="$EVENTLIBS -L$withval/lib64"
	  libeventpath=$withval/lib64:$libeventpath
        fi
      fi
      ;;
    esac ])
  option_old_libs=$LIBS
  AC_CHECK_LIB(event, event_base_new, [EVENTLIBS="$EVENTLIBS -levent"],
      AC_MSG_ERROR([event_base_new() not found. sock requires libevent.]),[$EVENTLIBS])
  AC_SUBST(EVENTLIBS)
  AC_SUBST(libeventpath)
  LIBS=$option_old_libs
])


dnl SYNOPSIS: OPTION_WITH_MAGIC([name],[default_integer],[description])
dnl EXAMPLE: OPTION_WITH_MAGIC([XYZPORT],[411],[default xyz port])
dnl sets default value of magic number XYZ for make and headers, 
dnl using second argument as default if not given by user
dnl and description.
dnl Good for getting default sizes and ports at config time
AC_DEFUN([OPTION_WITH_MAGIC], [
AC_ARG_WITH(
        $1,
        AS_HELP_STRING(
                [--with-$1@<:@=NNN@:>@],
                [Specify $1 $3 @<:@configure default=$2@:>@]
        ),
        [$1=$withval],
        [$1=$2; withval=$2]
)
$1=$withval
if printf "%d" "$withval" >/dev/null 2>&1; then
        :
else
        AC_MSG_ERROR([--with-$1 given non-integer input $withval])
fi
AC_DEFINE_UNQUOTED([$1],[$withval],[$3])
AC_SUBST([$1],[$$1])
])

dnl SYNOPSIS: OPTION_GITINFO
dnl dnl queries git for version hash and branch info.
AC_DEFUN([OPTION_GITINFO], [
	treetop=missing
	for ovis_top in `pwd` $ac_abs_confdir $ac_abs_confdir/.. $ac_abs_confdir/../..; do
		if test -f $ovis_top/m4/Ovis-top.m4; then
			treetop=`(cd "$ovis_top"
			pwd)`
			break
		fi
	done
	if test "$treetop" = "missing"; then
		AC_MSG_ERROR([Unable to locate top of ovis source tree.])
	fi	
if test -s $treetop/TAG.txt && test -s $treetop/SHA.txt; then
	AC_MSG_NOTICE([Using SHA.txt and TAG.txt from $treetop for version info. ])
	GITSHORT="$( cat $treetop/TAG.txt)"
	GITLONG="$( cat $treetop/SHA.txt)"
else
	AC_MSG_CHECKING([Missing SHA.txt or TAG.txt in $treetop. Trying git])
	if test "x`which git`" = "x"; then
	        GITSHORT="no_git_command"
	        GITLONG=$GITSHORT
		AC_MSG_RESULT([Faking it.])
	else
	        GITLONG="`git log -1 |grep ^commit | sed -e 's%commit %%'`"
	        GITSHORT="`git describe --tags`"
		AC_MSG_RESULT([ok.])
	fi
fi

AC_DEFINE_UNQUOTED([LDMS_GIT_LONG],["$GITLONG"],[Hash of last git commit])
AC_DEFINE_UNQUOTED([LDMS_GIT_SHORT],["$GITSHORT"],[Branch and hash mangle of last commit])
])

dnl SYNOPSIS: OVIS_PKGLIBDIR
dnl defines automake pkglibdir value for configure output
dnl and enables gcc color
AC_DEFUN([OVIS_PKGLIBDIR], [
AC_SUBST([pkglibdir],['${libdir}'/$PACKAGE])
AX_CHECK_COMPILE_FLAG([-fdiagnostics-color=auto], [
CFLAGS="$CFLAGS -fdiagnostics-color=auto"
])
])

dnl SYNOPSIS: OPTION_HOSTINFO
dnl build environment description
AC_DEFUN([OPTION_HOSTINFO], [
AC_CANONICAL_HOST
AC_CANONICAL_BUILD
LDMS_COMPILE_HOST_NAME=$ac_hostname
LDMS_COMPILE_HOST_CPU=$host_cpu
LDMS_COMPILE_HOST_OS=$host_os
AC_DEFINE_UNQUOTED([LDMS_COMPILE_HOST_NAME],["$LDMS_COMPILE_HOST_NAME"],[host where configured])
AC_DEFINE_UNQUOTED([LDMS_COMPILE_HOST_CPU],["$LDMS_COMPILE_HOST_CPU"],[cpu where configured])
AC_DEFINE_UNQUOTED([LDMS_COMPILE_HOST_OS],["$LDMS_COMPILE_HOST_OS"],[os where configured])
AC_DEFINE_UNQUOTED([LDMS_CONFIG_ARGS],["$ac_configure_args"],[configure input])
])

AC_DEFUN([OVIS_EXEC_SCRIPTS], [
	ovis_exec_scripts=""
	for i in "$@"; do
		ovis_exec_scripts="$ovis_exec_scripts $i"
		AC_CONFIG_FILES([$i],[chmod a+x $i])
	done
])

