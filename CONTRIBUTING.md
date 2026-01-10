# Contributing to RetractorDB

All types of contributions are encouraged and valued. See the [Table of Contents](#table-of-contents) for different ways to help and details about how this project handles them. Please make sure to read the relevant section before making your contribution. It will make it a lot easier for us maintainers and smooth out the experience for all involved. 

> And if you like the project, but just don't have time to contribute, that's fine. There are other easy ways to support the project and show your appreciation, which we would also be very happy about:
> - Star the project
> - Tweet about it
> - Refer this project in your project's readme
> - Mention the project at local meetups and tell your friends/colleagues

If there is an issue that this Guide is not covering - please do not hestitate to write mail to me and ask - michal@widera.com.pl

## Table of Contents

- [I Have a Question](#i-have-a-question)
  - [I Want To Contribute](#i-want-to-contribute)
  - [Reporting Bugs](#reporting-bugs)
  - [Suggesting Enhancements](#suggesting-enhancements)

## I Have a Question

> If you want to ask a question, we assume that you have read the available [Documentation](httpshttps://retractordb.gitbook.io/retractordb-docs/).

Before you ask a question, it is best to search for existing [Issues](https://github.com/michalwidera/retractordb/issues) that might help you. In case you have found a suitable issue and still need clarification, you can write your question in this issue. It is also advisable to search the internet for answers first.

If you then still feel the need to ask a question and need clarification, we recommend the following:

- Open an [Issue](https://github.com/michalwidera/retractordb/issues/new).
- Provide as much context as you can about what you're running into.
- Provide project and platform versions (nodejs, npm, etc), depending on what seems relevant.

We will then take care of the issue as soon as possible.

## I Want To Contribute

> ### Legal Notice <!-- omit in toc -->
> When contributing to this project, you must agree that you have authored 100% of the content, that you have the necessary rights to the content and that the content you contribute may be provided under the project licence.

### Reporting Bugs

#### Before Submitting a Bug Report

A good bug report shouldn't leave others needing to chase you up for more information. Therefore, we ask you to investigate carefully, collect information and describe the issue in detail in your report. Please complete the following steps in advance to help us fix any potential bug as fast as possible.

- Make sure that you are using the latest version.
- Determine if your bug is really a bug and not an error on your side e.g. using incompatible environment components/versions (Make sure that you have read the [documentation](httpshttps://retractordb.gitbook.io/retractordb-docs/). If you are looking for support, you might want to check [this section](#i-have-a-question)).
- To see if other users have experienced (and potentially already solved) the same issue you are having, check if there is not already a bug report existing for your bug or error in the [bug tracker](https://github.com/michalwidera/retractordb/issues?q=label%3Abug).
- Also make sure to search the internet (including Stack Overflow) to see if users outside of the GitHub community have discussed the issue.
- Collect information about the bug:
  - Stack trace (Traceback)
  - OS, Platform and Version (Windows, Linux, macOS, x86, ARM)
  - Version of the interpreter, compiler, SDK, runtime environment, package manager, depending on what seems relevant.
  - Possibly your input and the output
  - Can you reliably reproduce the issue? And can you also reproduce it with older versions?

#### How Do I Submit a Good Bug Report?

> You must never report security related issues, vulnerabilities or bugs including sensitive information to the issue tracker, or elsewhere in public. Instead sensitive bugs must be sent by email to <michal@retractordb.com>.

We use GitHub issues to track bugs and errors. If you run into an issue with the project:

- Open an [Issue](https://github.com/michalwidera/retractordb/issues/new). (Since we can't be sure at this point whether it is a bug or not, we ask you not to talk about a bug yet and not to label the issue.)
- Explain the behavior you would expect and the actual behavior.
- Please provide as much context as possible and describe the *reproduction steps* that someone else can follow to recreate the issue on their own. This usually includes your code. For good bug reports you should isolate the problem and create a reduced test case.
- Provide the information you collected in the previous section.

Once it's filed:

- The project team will label the issue accordingly.
- A team member will try to reproduce the issue with your provided steps. If there are no reproduction steps or no obvious way to reproduce the issue, the team will ask you for those steps and mark the issue as `needs-repro`. Bugs with the `needs-repro` tag will not be addressed until they are reproduced.
- If the team is able to reproduce the issue, it will be marked `needs-fix`, as well as possibly other tags (such as `critical`), and the issue will be left to be [implemented by someone](#your-first-code-contribution).

### Suggesting Enhancements

This section guides you through submitting an enhancement suggestion for RetractorDB, **including completely new features and minor improvements to existing functionality**. Following these guidelines will help maintainers and the community to understand your suggestion and find related suggestions.

#### Before Submitting an Enhancement

- Make sure that you are using the latest version.
- Read the [documentation](httpshttps://retractordb.gitbook.io/retractordb-docs/) carefully and find out if the functionality is already covered, maybe by an individual configuration.
- Perform a [search](https://github.com/michalwidera/retractordb/issues) to see if the enhancement has already been suggested. If it has, add a comment to the existing issue instead of opening a new one.
- Find out whether your idea fits with the scope and aims of the project. It's up to you to make a strong case to convince the project's developers of the merits of this feature. Keep in mind that we want features that will be useful to the majority of our users and not just a small subset. If you're just targeting a minority of users, consider writing an add-on/plugin library.

#### How Do I Submit a Good Enhancement Suggestion?

Enhancement suggestions are tracked as [GitHub issues](https://github.com/michalwidera/retractordb/issues).

- Use a **clear and descriptive title** for the issue to identify the suggestion.
- Provide a **step-by-step description of the suggested enhancement** in as many details as possible.
- **Describe the current behavior** and **explain which behavior you expected to see instead** and why. At this point you can also tell which alternatives do not work for you.
- You may want to **include screenshots or screen recordings** which help you demonstrate the steps or point out the part which the suggestion is related to. You can use [LICEcap](https://www.cockos.com/licecap/) to record GIFs on macOS and Windows, and the built-in [screen recorder in GNOME](https://help.gnome.org/users/gnome-help/stable/screen-shot-record.html.en) or [SimpleScreenRecorder](https://github.com/MaartenBaert/ssr) on Linux. <!-- this should only be included if the project has a GUI -->
- **Explain why this enhancement would be useful** to most RetractorDB users. You may also want to point out the other projects that solved it better and which could serve as inspiration.

## On AI use in RetractorDB
Guidelines for AI use when contributing to RetractorDB.

For security reports and other issues
If you asked an AI tool to find problems in curl, you must make sure to reveal this fact in your report.

You must also double-check the findings carefully before reporting them to us to validate that the issues are indeed existing and working exactly as the AI says. AI-based tools frequently generate inaccurate or fabricated results.

Further: it is rarely a good idea to just copy and paste an AI generated report to the project. Those generated reports typically are too wordy and rarely to the point (in addition to the common fabricated details). If you actually find a problem with an AI and you have verified it yourself to be true: write the report yourself and explain the problem as you have learned it. This makes sure the AI-generated inaccuracies and invented issues are filtered out early before they waste more people's time.

As we take security reports seriously, we investigate each report with priority. This work is both time and energy consuming and pulls us away from doing other meaningful work. Fake and otherwise made up security problems effectively prevent us from doing real project work and make us waste time and resources.

We ban users immediately who submit made up fake reports to the project.

### For pull requests - Code License Contamination Protection

When contributing content to the RetractorDB project, you give us permission to use it as-is and you must make sure you are allowed to distribute it to us. By submitting a change to us, you agree that the changes can and should be adopted by RetractorDB and get redistributed under the RetractorDB license. Authors should be explicitly aware that the burden is on them to ensure no unlicensed code is submitted to the project.

This is independent if AI is used or not.

When contributing a pull request you should of course always make sure that the proposal is good quality and a best effort that follows our guidelines. A basic rule of thumb is that if someone can spot that the contribution was made with the help of AI, you have more work to do.

We can accept code written with the help of AI into the project, but the code must still follow coding standards, be written clearly, be documented, feature test cases and adhere to all the normal requirements we have.

### For translation
Translation services help users write reports, texts and documentation in non-native languages and we encourage and welcome such contributors and contributions.

As AI-based translation tools sometimes have a way to make the output sound a little robotic and add an "AI tone" to the text, you may want to consider mentioning that you used such a tool. Failing to do so risks that maintainers wrongly dismiss translated texts as AI slop.

