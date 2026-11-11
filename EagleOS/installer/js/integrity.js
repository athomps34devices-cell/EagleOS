"use strict";

const INTEGRITY_MANIFEST = "integrity-manifest.json";

async function calculateHash(buffer) {
    const hashBuffer = await crypto.subtle.digest("SHA-256", buffer);

    return Array.from(new Uint8Array(hashBuffer))
        .map(byte => byte.toString(16).padStart(2, "0"))
        .join("");
}

async function verifyIntegrityFile(file) {
    try {
        const response = await fetch(
            new URL(file.path, document.baseURI),
            { cache: "no-store" }
        );

        if (!response.ok) {
            return {
                path: file.path,
                reason: `Unable to access file. HTTP ${response.status}.`
            };
        }

        const buffer = await response.arrayBuffer();
        const actualHash = await calculateHash(buffer);

        if (actualHash.toLowerCase() !== file.sha256.toLowerCase()) {
            return {
                path: file.path,
                reason: "SHA-256 checksum does not match."
            };
        }

        return null;
    } catch (error) {
        return {
            path: file.path,
            reason: error.message || "Unable to verify file."
        };
    }
}

async function verifyEagleOS() {
    const issues = [];

    try {
        const response = await fetch(
            new URL(INTEGRITY_MANIFEST, document.baseURI),
            { cache: "no-store" }
        );

        if (!response.ok) {
            throw new Error(
                `Unable to load integrity manifest. HTTP ${response.status}.`
            );
        }

        const manifest = await response.json();

        if (
            !manifest ||
            manifest.version !== 1 ||
            !Array.isArray(manifest.files) ||
            manifest.files.length === 0
        ) {
            throw new Error("The integrity manifest is invalid.");
        }

        for (const file of manifest.files) {
            if (
                !file ||
                typeof file.path !== "string" ||
                typeof file.sha256 !== "string" ||
                !/^[a-f0-9]{64}$/i.test(file.sha256)
            ) {
                issues.push({
                    path: file?.path || "Unknown file",
                    reason: "Invalid file entry in integrity manifest."
                });

                continue;
            }

            const issue = await verifyIntegrityFile(file);

            if (issue) {
                issues.push(issue);
            }
        }

        return {
            verified: issues.length === 0,
            issues
        };

    } catch (error) {
        return {
            verified: false,
            issues: [
                {
                    path: INTEGRITY_MANIFEST,
                    reason: error.message || "Integrity verification failed."
                }
            ]
        };
    }
}

window.EagleOSIntegrity = {
    verify: verifyEagleOS
};