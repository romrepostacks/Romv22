# Anonymous feedback relay

In-game **START → FEEDBACK** posts to this Cloudflare Worker, which files a GitHub issue for the
tester using a token only the Worker holds, so testers need no account. Until it's set up, the game
falls back to a pre-filled GitHub issue link, which does need an account.

## One-time setup (about 10 minutes, free)

1. **GitHub token.** Go to GitHub → Settings → Developer settings → Personal access tokens →
   **Fine-grained tokens** → Generate new token.
   - Repository access: **Only select repositories** → `romrepostacks/Romv22`
   - Permissions → Repository permissions → **Issues: Read and write**. Leave everything else as it is.
   - Pick an expiry date, generate the token, and copy it. You won't be shown it again.
2. **Cloudflare.** Sign up free at dash.cloudflare.com, then open **Workers & Pages** → **Create** →
   **Create Worker**. Name it `party-royale-feedback` → Deploy.
3. **Code.** Choose **Edit code**, replace everything with the contents of `relay/worker.js`, then **Deploy**.
4. **Settings.** Open the Worker's **Settings → Variables and Secrets** and add:
   - `GITHUB_TOKEN`, type **Secret**: the token from step 1
   - `REPO`, type Text: `romrepostacks/Romv22`
   - `ALLOW_ORIGIN`, type Text: `https://romrepostacks.github.io`
5. **Hook it up.** Copy the Worker's URL (`https://party-royale-feedback.<you>.workers.dev`) and set
   `FEEDBACK_ENDPOINT` to it in `js/app.js`, then push the change.

## Protections

- The token can only touch issues on this one repo, and it never reaches the browser.
- Only the game's own site may post (checked by origin).
- A hidden field traps bots.
- Each player can send once every 30 seconds.
- Text is capped at 2,000 characters, and @-mentions are defused so anonymous posts can't ping people.
