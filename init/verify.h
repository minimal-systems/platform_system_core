#ifndef VERIFY_H
#define VERIFY_H

#include <string>

std::string ComputeSHA256(const std::string &filepath);
bool VerifyPartition(const std::string &device, const std::string &mount_point,
		     const std::string &filesystem);
bool VerifySecureBootSHA(const std::string &computed_hash);
bool VerifySignature(const std::string &hash);
std::string GetSecureBootSHA();
std::string GetVerityKeyPath();

#endif // VERIFY_H
