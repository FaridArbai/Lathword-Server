#include "privateengine.h"

const char PrivateEngine::PRIVATE_KEY[] = "-----BEGIN RSA PRIVATE KEY-----\n"
                            "THIS IS SECRET, IF YOU WANT TO USE THIS SERVER FOR ANOTHER\n"
	       "PURPOSES YOU WILL HAVE TO RESET THIS VARIABLE AS WELL AS \n."
	       "THE CLIENT PUBLIC KEY."
                             "-----END RSA PRIVATE KEY-----";

string PrivateEngine::decrypt(const string& encr_b64){
    string encr;
    int encr_len;
    const char* encr_c;
    char* decr_c;
    string decr;

    BIO* private_bio = BIO_new_mem_buf(PRIVATE_KEY,(int)sizeof(PRIVATE_KEY));
    EVP_PKEY* private_key_evp = PEM_read_bio_PrivateKey(private_bio,NULL,NULL,NULL);
    RSA* private_key_rsa = EVP_PKEY_get1_RSA(private_key_evp);

    encr = Base64::decode(encr_b64);
    encr_len = encr.length();
    encr_c = encr.c_str();

    decr_c = (char*)malloc(RSA_size(private_key_rsa));

    RSA_private_decrypt(encr_len,
                       (unsigned char*)encr_c,
                       (unsigned char*)decr_c,
                       private_key_rsa,
                       RSA_PKCS1_OAEP_PADDING);

    decr = string(decr_c);
    free(decr_c);

    return decr;
}

string PrivateEngine::sign(const string& msg){
    string hash = PrivateEngine::sha256(msg);
    string signature = PrivateEngine::encryptHash(hash);

    return signature;
}

string PrivateEngine::sha256(const string& msg){
    string hash_str;
    const char* msg_c = msg.c_str();
    int msg_len = msg.length();
    unsigned char hash[SHA256_DIGEST_LENGTH+1];
    SHA256_CTX sha256;

    SHA256_Init(&sha256);
    SHA256_Update(&sha256,msg_c,msg_len);
    SHA256_Final(hash,&sha256);
    hash[SHA256_DIGEST_LENGTH] = (unsigned char)'\0';

    hash_str = string((const char*)hash);

    return hash_str;
}

string PrivateEngine::encryptHash(const string& hash){
    int hash_len = hash.length();
    const char* hash_c = hash.c_str();
    char* encr_c;
    int encr_len;
    string encr_b64;

    BIO* private_bio = BIO_new_mem_buf(PRIVATE_KEY,(int)sizeof(PRIVATE_KEY));
    EVP_PKEY* private_key_evp = PEM_read_bio_PrivateKey(private_bio,NULL,NULL,NULL);
    RSA* private_key_rsa = EVP_PKEY_get1_RSA(private_key_evp);

    encr_c = (char*)malloc(RSA_size(private_key_rsa));

    encr_len = RSA_private_encrypt(hash_len+1,
                                   (unsigned char*)hash_c,
                                   (unsigned char*)encr_c,
                                   private_key_rsa,
                                   RSA_PKCS1_PADDING);

    encr_b64 = Base64::encode((const unsigned char*)encr_c,encr_len);
    free(encr_c);

    return encr_b64;
}


























































































