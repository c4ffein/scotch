use hegel::generators as gs;
use hegel::{Hegel, TestCase};
use std::ffi::CStr;
use std::os::raw::{c_char, c_int};

/// Opaque handle passed to C test functions.
pub struct HegelTestCase {
    tc: TestCase,
}

/// C test function signature: void test_fn(hegel_testcase *tc).
/// Uses C-unwind so that Rust panics (from hegel_fail, hegel_assume,
/// or internal hegel StopTest) can unwind back through the C frame.
type CTestFn = unsafe extern "C-unwind" fn(*mut HegelTestCase);

unsafe extern "C" {
    fn SCOTCH_randomSeed(seed: c_int);
    fn SCOTCH_randomReset();
}

/// Reset Scotch's global random state before each test case.
/// This ensures deterministic behavior across hegel replays.
fn scotch_reset() {
    unsafe {
        SCOTCH_randomSeed(42);
        SCOTCH_randomReset();
    }
}

/// Run a hegel property test that calls a C function for each test case.
///
/// # Safety
/// `test_fn` must be a valid C function pointer.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn hegel_run_test(test_fn: CTestFn) {
    Hegel::new(move |tc: TestCase| {
        scotch_reset();
        let mut htc = HegelTestCase { tc };
        unsafe { test_fn(&mut htc) };
    })
    .run();
}

/// Run a hegel property test with a custom number of test cases.
///
/// # Safety
/// `test_fn` must be a valid C function pointer.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn hegel_run_test_n(test_fn: CTestFn, n_cases: u64) {
    Hegel::new(move |tc: TestCase| {
        scotch_reset();
        let mut htc = HegelTestCase { tc };
        unsafe { test_fn(&mut htc) };
    })
    .settings(hegel::Settings::new().test_cases(n_cases))
    .run();
}

/// Fail the current test case with a message. This triggers a Rust panic
/// which hegel catches for shrinking. Use this instead of abort() in C tests.
///
/// # Safety
/// `msg` must be a valid null-terminated C string, or NULL.
#[unsafe(no_mangle)]
pub unsafe extern "C-unwind" fn hegel_fail(msg: *const c_char) {
    let message = if msg.is_null() {
        "hegel_fail called".to_string()
    } else {
        unsafe { CStr::from_ptr(msg) }
            .to_string_lossy()
            .into_owned()
    };
    panic!("{}", message);
}

/// Assert a condition. If false, fail with a message (triggers shrinking).
///
/// # Safety
/// `msg` must be a valid null-terminated C string, or NULL.
#[unsafe(no_mangle)]
pub unsafe extern "C-unwind" fn hegel_assert(
    condition: c_int,
    msg: *const c_char,
) {
    if condition == 0 {
        unsafe { hegel_fail(msg) };
    }
}

/// Draw a random integer in [min_val, max_val].
///
/// # Safety
/// `tc` must be a valid pointer obtained from a hegel test callback.
#[unsafe(no_mangle)]
pub unsafe extern "C-unwind" fn hegel_draw_int(
    tc: *mut HegelTestCase,
    min_val: c_int,
    max_val: c_int,
) -> c_int {
    let htc = unsafe { &*tc };
    htc.tc
        .draw(gs::integers::<c_int>().min_value(min_val).max_value(max_val))
}

/// Draw a random i64 in [min_val, max_val].
///
/// # Safety
/// `tc` must be a valid pointer obtained from a hegel test callback.
#[unsafe(no_mangle)]
pub unsafe extern "C-unwind" fn hegel_draw_i64(
    tc: *mut HegelTestCase,
    min_val: i64,
    max_val: i64,
) -> i64 {
    let htc = unsafe { &*tc };
    htc.tc
        .draw(gs::integers::<i64>().min_value(min_val).max_value(max_val))
}

/// Draw a random u64 in [min_val, max_val].
///
/// # Safety
/// `tc` must be a valid pointer obtained from a hegel test callback.
#[unsafe(no_mangle)]
pub unsafe extern "C-unwind" fn hegel_draw_u64(
    tc: *mut HegelTestCase,
    min_val: u64,
    max_val: u64,
) -> u64 {
    let htc = unsafe { &*tc };
    htc.tc
        .draw(gs::integers::<u64>().min_value(min_val).max_value(max_val))
}

/// Draw a random usize in [min_val, max_val].
///
/// # Safety
/// `tc` must be a valid pointer obtained from a hegel test callback.
#[unsafe(no_mangle)]
pub unsafe extern "C-unwind" fn hegel_draw_usize(
    tc: *mut HegelTestCase,
    min_val: usize,
    max_val: usize,
) -> usize {
    let htc = unsafe { &*tc };
    htc.tc
        .draw(gs::integers::<usize>().min_value(min_val).max_value(max_val))
}

/// Assume a condition. If false, this test case is discarded (not a failure).
///
/// # Safety
/// `tc` must be a valid pointer obtained from a hegel test callback.
#[unsafe(no_mangle)]
pub unsafe extern "C-unwind" fn hegel_assume(tc: *mut HegelTestCase, condition: c_int) {
    let htc = unsafe { &*tc };
    htc.tc.assume(condition != 0);
}
