use proc_macro2::Span;
use quote::quote;
use syn::{
    parse, parse_macro_input, spanned::Spanned, FnArg, ItemFn, ReturnType, Type, Visibility,
};

use proc_macro::TokenStream;

/// SyterKit ROM stage function entry.
#[proc_macro_attribute]
pub fn entry(args: TokenStream, input: TokenStream) -> TokenStream {
    let f = parse_macro_input!(input as ItemFn);

    // check the function arguments
    if f.sig.inputs.len() != 2 {
        return parse::Error::new(
            f.sig.inputs.last().unwrap().span(),
            "`#[entry]` function should include exactly two parameters",
        )
        .to_compile_error()
        .into();
    }

    for arg in &f.sig.inputs {
        match arg {
            FnArg::Receiver(_) => {
                return parse::Error::new(arg.span(), "invalid argument")
                    .to_compile_error()
                    .into();
            }
            FnArg::Typed(t) => {
                if let Type::Path(_p) = &*t.ty {
                    // empty
                } else {
                    return parse::Error::new(t.ty.span(), "argument type must be a path")
                        .to_compile_error()
                        .into();
                }
            }
        }
    }

    // check the function signature
    let valid_signature = f.sig.constness.is_none()
        && f.sig.asyncness.is_none()
        && f.vis == Visibility::Inherited
        && f.sig.abi.is_none()
        && f.sig.generics.params.is_empty()
        && f.sig.generics.where_clause.is_none()
        && f.sig.variadic.is_none()
        && match f.sig.output {
            ReturnType::Default => true,
            _ => false,
        };

    if !valid_signature {
        return parse::Error::new(
            f.span(),
            "`#[entry]` function must have signature `[unsafe] fn(p: Peripherals, c: Clocks)`",
        )
        .to_compile_error()
        .into();
    }

    if !args.is_empty() {
        return parse::Error::new(Span::call_site(), "This attribute accepts no arguments")
            .to_compile_error()
            .into();
    }

    let attrs = f.attrs;
    let unsafety = f.sig.unsafety;
    let args = f.sig.inputs;
    let stmts = f.block.stmts;
    let ret = f.sig.output;

    quote!(
        #[export_name = "main"]
        pub fn main() {
            let (allwinner_rt_p, c) = ::syterkit::allwinner_rt::__rom_init_params();
            let (p, uart0, tx, rx) = ::syterkit::Peripherals::configure_uart0(allwinner_rt_p);
            let mut serial = ::syterkit::allwinner_hal::uart::Serial::new(uart0, (tx, rx), ::syterkit::allwinner_hal::uart::Config::default(), &c, &p.ccu);
            unsafe {
                *::syterkit::CONSOLE.lock() = Some(::syterkit::SyterKitConsole { inner: serial })
            };
            unsafe { __syterkit_macros__main(p, c) }
        }
        #[allow(non_snake_case)]
        #[inline]
        #(#attrs)*
        #unsafety fn __syterkit_macros__main(#args) #ret {
            #(#stmts)*
        }
    )
    .into()
}
