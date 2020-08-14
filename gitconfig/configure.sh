#!/bin/bash
cp * ../.git/hooks
git config core.editor "vim"
git config commit.template ./.git/hooks/git-commit-template
