
use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use std::fmt::Write;  // <-- ¡Esta línea faltaba!
use anyhow::Result;
use blowfish::cipher::{block_padding::NoPadding, BlockDecryptMut, KeyIvInit};
use blowfish::Blowfish;
use cbc::Decryptor as BlowfishCbcDec;

const DEEZER_SECRET: &str = "g4el58wc0zvf9na1";
const CHUNK_SIZE: usize = 2048;
const BLOWFISH_BLOCK_SIZE: usize = 8;
const DEEZER_BLOWFISH_IV: [u8; 8] = [0, 1, 2, 3, 4, 5, 6, 7];

fn derive_blowfish_key(track_id: &str) -> [u8; 16] {
    let id_md5 = md5_hex(track_id.as_bytes());
    let id_md5_bytes = id_md5.as_bytes();
    let secret_bytes = DEEZER_SECRET.as_bytes();
    let mut key = [0u8; 16];

    for i in 0..16 {
        key[i] = id_md5_bytes[i] ^ id_md5_bytes[i + 16] ^ secret_bytes[i];
    }

    key
}

fn md5_hex(bytes: &[u8]) -> String {
    let digest = md5::compute(bytes);
    let mut output = String::with_capacity(32);
    for byte in digest.0 {
        let _ = write!(&mut output, "{byte:02x}");
    }
    output
}

fn decrypt_chunk_in_place(key: &[u8], chunk_index: usize, chunk: &mut [u8]) -> Result<()> {
    if chunk_index % 3 != 0 {
        return Ok(());
    }

    let decryptable_len = chunk.len() - (chunk.len() % BLOWFISH_BLOCK_SIZE);
    if decryptable_len == 0 {
        return Ok(());
    }

    let chunk_prefix = &mut chunk[..decryptable_len];
    let cipher = BlowfishCbcDec::<Blowfish>::new_from_slices(key, &DEEZER_BLOWFISH_IV)
        .map_err(|_| anyhow::anyhow!("failed to initialize Blowfish-CBC decryptor"))?;
    cipher
        .decrypt_padded_mut::<NoPadding>(chunk_prefix)
        .map_err(|_| anyhow::anyhow!("failed to decrypt chunk using Blowfish-CBC"))?;

    Ok(())
}

#[no_mangle]
pub extern "C" fn decrypt_audio(
    track_id: *const c_char,
    encrypted_data: *const u8,
    data_len: usize,
    out_len: *mut usize,
) -> *mut u8 {
    let track_id_str = unsafe { CStr::from_ptr(track_id).to_str().unwrap() };
    let encrypted_slice = unsafe { std::slice::from_raw_parts(encrypted_data, data_len) };
    
    let key = derive_blowfish_key(track_id_str);
    let mut output = Vec::with_capacity(encrypted_slice.len());
    output.extend_from_slice(encrypted_slice);
    
    for (chunk_index, chunk) in output.chunks_mut(CHUNK_SIZE).enumerate() {
        let _ = decrypt_chunk_in_place(&key, chunk_index, chunk);
    }
    
    unsafe { *out_len = output.len() };
    let ptr = output.as_mut_ptr();
    std::mem::forget(output);
    ptr
}

#[no_mangle]
pub extern "C" fn free_decrypted(ptr: *mut u8) {
    if !ptr.is_null() {
        unsafe { 
            let _ = Vec::from_raw_parts(ptr, 0, 0);
        }
    }
}
