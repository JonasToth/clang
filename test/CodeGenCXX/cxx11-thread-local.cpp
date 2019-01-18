// RUN: %clang_cc1 -std=c++11 -emit-llvm %s -o - -triple x86_64-linux-gnu | FileCheck --check-prefix=CHECK --check-prefix=LINUX %s
// RUN: %clang_cc1 -std=c++11 -emit-llvm %s -O2 -disable-llvm-passes -o - -triple x86_64-linux-gnu | FileCheck --check-prefix=CHECK --check-prefix=LINUX --check-prefix=CHECK-OPT %s
// RUN: %clang_cc1 -std=c++11 -femulated-tls -emit-llvm %s -o - \
// RUN:     -triple x86_64-linux-gnu 2>&1 | FileCheck --check-prefix=CHECK --check-prefix=LINUX %s
// RUN: %clang_cc1 -std=c++11 -emit-llvm %s -o - -triple x86_64-apple-darwin12 | FileCheck --check-prefix=CHECK --check-prefix=DARWIN %s

int f();
int g();

// LINUX-DAG: @a = thread_local global i32 0
// DARWIN-DAG: @a = internal thread_local global i32 0
thread_local int a = f();
extern thread_local int b;
// CHECK-DAG: @c = global i32 0
int c = b;
// CHECK-DAG: @_ZL1d = internal thread_local global i32 0
static thread_local int d = g();

struct U { static thread_local int m; };
// LINUX-DAG: @_ZN1U1mE = thread_local global i32 0
// DARWIN-DAG: @_ZN1U1mE = internal thread_local global i32 0
thread_local int U::m = f();

namespace MismatchedInitType {
  // Check that we don't crash here when we're forced to create a new global
  // variable (with a different type) when we add the initializer.
  union U {
    int a;
    float f;
    constexpr U() : f(0.0) {}
  };
  static thread_local U u;
  void *p = &u;
}

template<typename T> struct V { static thread_local int m; };
template<typename T> thread_local int V<T>::m = g();

template<typename T> struct W { static thread_local int m; };
template<typename T> thread_local int W<T>::m = 123;

struct Dtor { ~Dtor(); };
template<typename T> struct X { static thread_local Dtor m; };
template<typename T> thread_local Dtor X<T>::m;

// CHECK-DAG: @e = global
void *e = V<int>::m + W<int>::m + &X<int>::m;

template thread_local int V<float>::m;
template thread_local int W<float>::m;
template thread_local Dtor X<float>::m;

extern template thread_local int V<char>::m;
extern template thread_local int W<char>::m;
extern template thread_local Dtor X<char>::m;

void *e2 = V<char>::m + W<char>::m + &X<char>::m;

// CHECK-DAG: @_ZN1VIiE1mE = linkonce_odr thread_local global i32 0
// CHECK-DAG: @_ZN1WIiE1mE = linkonce_odr thread_local global i32 123
// CHECK-DAG: @_ZN1XIiE1mE = linkonce_odr thread_local global {{.*}}
// CHECK-DAG: @_ZN1VIfE1mE = weak_odr thread_local global i32 0
// CHECK-DAG: @_ZN1WIfE1mE = weak_odr thread_local global i32 123
// CHECK-DAG: @_ZN1XIfE1mE = weak_odr thread_local global {{.*}}

// CHECK-DAG: @_ZZ1fvE1n = internal thread_local global i32 0

// CHECK-DAG: @_ZGVZ1fvE1n = internal thread_local global i8 0

// CHECK-DAG: @_ZZ8tls_dtorvE1s = internal thread_local global
// CHECK-DAG: @_ZGVZ8tls_dtorvE1s = internal thread_local global i8 0

// CHECK-DAG: @_ZZ8tls_dtorvE1t = internal thread_local global
// CHECK-DAG: @_ZGVZ8tls_dtorvE1t = internal thread_local global i8 0

// CHECK-DAG: @_ZZ8tls_dtorvE1u = internal thread_local global
// CHECK-DAG: @_ZGVZ8tls_dtorvE1u = internal thread_local global i8 0
// CHECK-DAG: @_ZGRZ8tls_dtorvE1u_ = internal thread_local global

// CHECK-DAG: @_ZGVN1VIiE1mE = linkonce_odr thread_local global i64 0

// CHECK-DAG: @__tls_guard = internal thread_local global i8 0

// CHECK-DAG: @llvm.global_ctors = appending global {{.*}} @[[GLOBAL_INIT:[^ ]*]]

// LINUX-DAG: @_ZTH1a = alias void (), void ()* @__tls_init
// DARWIN-DAG: @_ZTH1a = internal alias void (), void ()* @__tls_init
// CHECK-DAG: @_ZTHL1d = internal alias void (), void ()* @__tls_init
// LINUX-DAG: @_ZTHN1U1mE = alias void (), void ()* @__tls_init
// DARWIN-DAG: @_ZTHN1U1mE = internal alias void (), void ()* @__tls_init
// CHECK-DAG: @_ZTHN1VIiE1mE = linkonce_odr alias void (), void ()* @[[V_M_INIT:[^, ]*]]
// CHECK-NOT: @_ZTHN1WIiE1mE =
// CHECK-DAG: @_ZTHN1XIiE1mE = linkonce_odr alias void (), void ()* @[[X_M_INIT:[^, ]*]]
// CHECK-DAG: @_ZTHN1VIfE1mE = weak_odr alias void (), void ()* @[[VF_M_INIT:[^, ]*]]
// CHECK-NOT: @_ZTHN1WIfE1mE =
// CHECK-DAG: @_ZTHN1XIfE1mE = weak_odr alias void (), void ()* @[[XF_M_INIT:[^, ]*]]


// Individual variable initialization functions:

// CHECK: define {{.*}} @[[A_INIT:.*]]()
// CHECK: call i32 @_Z1fv()
// CHECK-NEXT: store i32 {{.*}}, i32* @a, align 4

// CHECK-LABEL: define i32 @_Z1fv()
int f() {
  // CHECK: %[[GUARD:.*]] = load i8, i8* @_ZGVZ1fvE1n, align 1
  // CHECK: %[[NEED_INIT:.*]] = icmp eq i8 %[[GUARD]], 0
  // CHECK: br i1 %[[NEED_INIT]]

  // CHECK: %[[CALL:.*]] = call i32 @_Z1gv()
  // CHECK: store i32 %[[CALL]], i32* @_ZZ1fvE1n, align 4
  // CHECK: store i8 1, i8* @_ZGVZ1fvE1n
  // CHECK: br label
  static thread_local int n = g();

  // CHECK: load i32, i32* @_ZZ1fvE1n, align 4
  return n;
}

// CHECK: define {{.*}} @[[C_INIT:.*]]()
// LINUX: call i32* @_ZTW1b()
// DARWIN: call cxx_fast_tlscc i32* @_ZTW1b()
// CHECK-NEXT: load i32, i32* %{{.*}}, align 4
// CHECK-NEXT: store i32 %{{.*}}, i32* @c, align 4

// LINUX-LABEL: define weak_odr hidden i32* @_ZTW1b()
// LINUX: br i1 icmp ne (void ()* @_ZTH1b, void ()* null),
// not null:
// LINUX: call void @_ZTH1b()
// LINUX: br label
// finally:
// LINUX: ret i32* @b
// DARWIN-LABEL: declare cxx_fast_tlscc i32* @_ZTW1b()
// There is no definition of the thread wrapper on Darwin for external TLV.

// CHECK: define {{.*}} @[[D_INIT:.*]]()
// CHECK: call i32 @_Z1gv()
// CHECK-NEXT: store i32 %{{.*}}, i32* @_ZL1d, align 4

// CHECK: define {{.*}} @[[U_M_INIT:.*]]()
// CHECK: call i32 @_Z1fv()
// CHECK-NEXT: store i32 %{{.*}}, i32* @_ZN1U1mE, align 4

// CHECK: define {{.*}} @[[E_INIT:.*]]()
// LINUX: call i32* @_ZTWN1VIiE1mE()
// DARWIN: call cxx_fast_tlscc i32* @_ZTWN1VIiE1mE()
// CHECK-NEXT: load i32, i32* %{{.*}}, align 4
// LINUX: call {{.*}}* @_ZTWN1XIiE1mE()
// DARWIN: call cxx_fast_tlscc {{.*}}* @_ZTWN1XIiE1mE()
// CHECK: store {{.*}} @e

// LINUX-LABEL: define weak_odr hidden i32* @_ZTWN1VIiE1mE()
// DARWIN-LABEL: define weak_odr hidden cxx_fast_tlscc i32* @_ZTWN1VIiE1mE()
// LINUX: call void @_ZTHN1VIiE1mE()
// DARWIN: call cxx_fast_tlscc void @_ZTHN1VIiE1mE()
// CHECK: ret i32* @_ZN1VIiE1mE

// LINUX-LABEL: define weak_odr hidden i32* @_ZTWN1WIiE1mE()
// DARWIN-LABEL: define weak_odr hidden cxx_fast_tlscc i32* @_ZTWN1WIiE1mE()
// CHECK-NOT: call
// CHECK: ret i32* @_ZN1WIiE1mE

// LINUX-LABEL: define weak_odr hidden {{.*}}* @_ZTWN1XIiE1mE()
// DARWIN-LABEL: define weak_odr hidden cxx_fast_tlscc {{.*}}* @_ZTWN1XIiE1mE()
// LINUX: call void @_ZTHN1XIiE1mE()
// DARWIN: call cxx_fast_tlscc void @_ZTHN1XIiE1mE()
// CHECK: ret {{.*}}* @_ZN1XIiE1mE

// LINUX: define internal void @[[VF_M_INIT]]()
// DARWIN: define internal cxx_fast_tlscc void @[[VF_M_INIT]]()
// LINUX-SAME: comdat($_ZN1VIfE1mE)
// DARWIN-NOT: comdat
// CHECK: load i8, i8* bitcast (i64* @_ZGVN1VIfE1mE to i8*)
// CHECK: %[[VF_M_INITIALIZED:.*]] = icmp eq i8 %{{.*}}, 0
// CHECK: br i1 %[[VF_M_INITIALIZED]],
// need init:
// CHECK: call i32 @_Z1gv()
// CHECK: store i32 %{{.*}}, i32* @_ZN1VIfE1mE, align 4
// CHECK: store i64 1, i64* @_ZGVN1VIfE1mE
// CHECK: br label

// LINUX: define internal void @[[XF_M_INIT]]()
// DARWIN: define internal cxx_fast_tlscc void @[[XF_M_INIT]]()
// LINUX-SAME: comdat($_ZN1XIfE1mE)
// DARWIN-NOT: comdat
// CHECK: load i8, i8* bitcast (i64* @_ZGVN1XIfE1mE to i8*)
// CHECK: %[[XF_M_INITIALIZED:.*]] = icmp eq i8 %{{.*}}, 0
// CHECK: br i1 %[[XF_M_INITIALIZED]],
// need init:
// LINUX: call {{.*}}__cxa_thread_atexit
// DARWIN: call {{.*}}_tlv_atexit
// CHECK: store i64 1, i64* @_ZGVN1XIfE1mE
// CHECK: br label

// LINUX: declare i32 @__cxa_thread_atexit(void (i8*)*, i8*, i8*)
// DARWIN: declare i32 @_tlv_atexit(void (i8*)*, i8*, i8*)

// DARWIN: declare cxx_fast_tlscc i32* @_ZTWN1VIcE1mE()
// LINUX: define weak_odr hidden i32* @_ZTWN1VIcE1mE()
// LINUX-NOT: comdat
// LINUX: br i1 icmp ne (void ()* @_ZTHN1VIcE1mE,
// LINUX: call void @_ZTHN1VIcE1mE()
// LINUX: ret i32* @_ZN1VIcE1mE

// DARWIN: declare cxx_fast_tlscc i32* @_ZTWN1WIcE1mE()
// LINUX: define weak_odr hidden i32* @_ZTWN1WIcE1mE()
// LINUX-NOT: comdat
// LINUX: br i1 icmp ne (void ()* @_ZTHN1WIcE1mE,
// LINUX: call void @_ZTHN1WIcE1mE()
// LINUX: ret i32* @_ZN1WIcE1mE

// DARWIN: declare cxx_fast_tlscc {{.*}}* @_ZTWN1XIcE1mE()
// LINUX: define weak_odr hidden {{.*}}* @_ZTWN1XIcE1mE()
// LINUX-NOT: comdat
// LINUX: br i1 icmp ne (void ()* @_ZTHN1XIcE1mE,
// LINUX: call void @_ZTHN1XIcE1mE()
// LINUX: ret {{.*}}* @_ZN1XIcE1mE

struct S { S(); ~S(); };
struct T { ~T(); };

// CHECK-LABEL: define void @_Z8tls_dtorv()
void tls_dtor() {
  // CHECK: load i8, i8* @_ZGVZ8tls_dtorvE1s
  // CHECK: call void @_ZN1SC1Ev(%struct.S* @_ZZ8tls_dtorvE1s)
  // LINUX: call i32 @__cxa_thread_atexit({{.*}}@_ZN1SD1Ev {{.*}} @_ZZ8tls_dtorvE1s{{.*}} @__dso_handle
  // DARWIN: call i32 @_tlv_atexit({{.*}}@_ZN1SD1Ev {{.*}} @_ZZ8tls_dtorvE1s{{.*}} @__dso_handle
  // CHECK: store i8 1, i8* @_ZGVZ8tls_dtorvE1s
  static thread_local S s;

  // CHECK: load i8, i8* @_ZGVZ8tls_dtorvE1t
  // CHECK-NOT: _ZN1T
  // LINUX: call i32 @__cxa_thread_atexit({{.*}}@_ZN1TD1Ev {{.*}}@_ZZ8tls_dtorvE1t{{.*}} @__dso_handle
  // DARWIN: call i32 @_tlv_atexit({{.*}}@_ZN1TD1Ev {{.*}}@_ZZ8tls_dtorvE1t{{.*}} @__dso_handle
  // CHECK: store i8 1, i8* @_ZGVZ8tls_dtorvE1t
  static thread_local T t;

  // CHECK: load i8, i8* @_ZGVZ8tls_dtorvE1u
  // CHECK: call void @_ZN1SC1Ev(%struct.S* @_ZGRZ8tls_dtorvE1u_)
  // LINUX: call i32 @__cxa_thread_atexit({{.*}}@_ZN1SD1Ev {{.*}} @_ZGRZ8tls_dtorvE1u_{{.*}} @__dso_handle
  // DARWIN: call i32 @_tlv_atexit({{.*}}@_ZN1SD1Ev {{.*}} @_ZGRZ8tls_dtorvE1u_{{.*}} @__dso_handle
  // CHECK: store i8 1, i8* @_ZGVZ8tls_dtorvE1u
  static thread_local const S &u = S();
}

// CHECK: define {{.*}} @_Z7PR15991v(
int PR15991() {
  thread_local int n;
  auto l = [] { return n; };
  return l();
}

struct PR19254 {
  static thread_local int n;
  int f();
};
// CHECK: define {{.*}} @_ZN7PR192541fEv(
int PR19254::f() {
  // LINUX: call void @_ZTHN7PR192541nE(
  // DARWIN: call cxx_fast_tlscc i32* @_ZTWN7PR192541nE(
  return this->n;
}

namespace {
thread_local int anon_i{1};
}
void set_anon_i() {
  anon_i = 2;
}
// LINUX-LABEL: define internal i32* @_ZTWN12_GLOBAL__N_16anon_iE()
// DARWIN-LABEL: define internal cxx_fast_tlscc i32* @_ZTWN12_GLOBAL__N_16anon_iE()

// LINUX: define internal void @[[V_M_INIT]]()
// DARWIN: define internal cxx_fast_tlscc void @[[V_M_INIT]]()
// LINUX-SAME: comdat($_ZN1VIiE1mE)
// DARWIN-NOT: comdat
// CHECK: load i8, i8* bitcast (i64* @_ZGVN1VIiE1mE to i8*)
// CHECK: %[[V_M_INITIALIZED:.*]] = icmp eq i8 %{{.*}}, 0
// CHECK: br i1 %[[V_M_INITIALIZED]],
// need init:
// CHECK: call i32 @_Z1gv()
// CHECK: store i32 %{{.*}}, i32* @_ZN1VIiE1mE, align 4
// CHECK: store i64 1, i64* @_ZGVN1VIiE1mE
// CHECK: br label

// LINUX: define internal void @[[X_M_INIT]]()
// DARWIN: define internal cxx_fast_tlscc void @[[X_M_INIT]]()
// LINUX-SAME: comdat($_ZN1XIiE1mE)
// DARWIN-NOT: comdat
// CHECK: load i8, i8* bitcast (i64* @_ZGVN1XIiE1mE to i8*)
// CHECK: %[[X_M_INITIALIZED:.*]] = icmp eq i8 %{{.*}}, 0
// CHECK: br i1 %[[X_M_INITIALIZED]],
// need init:
// LINUX: call {{.*}}__cxa_thread_atexit
// DARWIN: call {{.*}}_tlv_atexit
// CHECK: store i64 1, i64* @_ZGVN1XIiE1mE
// CHECK: br label

// CHECK: define {{.*}}@[[GLOBAL_INIT:.*]]()
// CHECK: call void @[[C_INIT]]()
// CHECK: call void @[[E_INIT]]()


// CHECK: define {{.*}}@__tls_init()
// CHECK: load i8, i8* @__tls_guard
// CHECK: %[[NEED_TLS_INIT:.*]] = icmp eq i8 %{{.*}}, 0
// CHECK: br i1 %[[NEED_TLS_INIT]],
// init:
// CHECK: store i8 1, i8* @__tls_guard
// CHECK-OPT: call {}* @llvm.invariant.start.p0i8(i64 1, i8* @__tls_guard)
// CHECK-NOT: call void @[[V_M_INIT]]()
// CHECK: call void @[[A_INIT]]()
// CHECK-NOT: call void @[[V_M_INIT]]()
// CHECK: call void @[[D_INIT]]()
// CHECK-NOT: call void @[[V_M_INIT]]()
// CHECK: call void @[[U_M_INIT]]()
// CHECK-NOT: call void @[[V_M_INIT]]()


// LINUX: define weak_odr hidden i32* @_ZTW1a()
// DARWIN: define cxx_fast_tlscc i32* @_ZTW1a()
// LINUX:   call void @_ZTH1a()
// DARWIN: call cxx_fast_tlscc void @_ZTH1a()
// CHECK:   ret i32* @a
// CHECK: }


// LINUX: declare extern_weak void @_ZTH1b() [[ATTR:#[0-9]+]]


// LINUX-LABEL: define internal i32* @_ZTWL1d()
// DARWIN-LABEL: define internal cxx_fast_tlscc i32* @_ZTWL1d()
// LINUX: call void @_ZTHL1d()
// DARWIN: call cxx_fast_tlscc void @_ZTHL1d()
// CHECK: ret i32* @_ZL1d

// LINUX-LABEL: define weak_odr hidden i32* @_ZTWN1U1mE()
// DARWIN-LABEL: define cxx_fast_tlscc i32* @_ZTWN1U1mE()
// LINUX: call void @_ZTHN1U1mE()
// DARWIN: call cxx_fast_tlscc void @_ZTHN1U1mE()
// CHECK: ret i32* @_ZN1U1mE

// LINUX: attributes [[ATTR]] = { {{.+}} }
